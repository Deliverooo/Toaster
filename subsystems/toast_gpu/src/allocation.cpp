#include "toast_gpu/allocation.hpp"
#include "toast_gpu/allocation.hpp"

#include <ranges>

#include "toast_lib/pool.hpp"

#include <vma/vk_mem_alloc.h>

namespace toaster::gpu
{
	struct VirtualBlock
	{
		VmaVirtualBlock block{nullptr};
	};

	struct VirtualAllocation
	{
		VmaVirtualAllocation allocation{nullptr};
		VirtualBlockHandle   block{nullptr}; // Use a virtual block handle to check validity

		uint64 offset{0u}; // Offset at which this allocation was made into the virtual block
		uint64 size{0u};   // Size of the allocation
	};

	struct AllocationContext
	{
		GPUContext *            gpuCtx{nullptr};
		Pool<VirtualBlock>      virtualBlocks;
		Pool<VirtualAllocation> virtualAllocations;
	};

	auto allocation_ctx_create(GPUContext *p_gpu_ctx) -> AllocationContext *
	{
		AllocationContext *ctx{new AllocationContext{}};

		ctx->gpuCtx = p_gpu_ctx;

		ctx->virtualBlocks.setDestructorFn(+[](VirtualBlock *p_data, void *) -> void
		{
			vmaDestroyVirtualBlock(p_data->block);
		});

		ctx->virtualAllocations.setDestructorUserData(ctx);
		ctx->virtualAllocations.setDestructorFn(+[](VirtualAllocation *p_data, void *p_ctx) -> void
		{
			auto ctx{static_cast<AllocationContext *>(p_ctx)};
			if (const VirtualBlock *block{ctx->virtualBlocks.tryGet(p_data->block)})
				vmaVirtualFree(block->block, p_data->allocation);
			else
			{
				TST_PERMA_ASSERT_MSG(false, "Attempted to free virtual allocation after destroying associated block!");
			}
		});

		return ctx;
	}

	auto allocation_ctx_destroy(AllocationContext *p_ctx) -> void
	{
		p_ctx->virtualAllocations.clear();
		p_ctx->virtualBlocks.clear();

		delete p_ctx;
	}

	auto virtual_block_create(AllocationContext *p_ctx, uint64 p_size) -> VirtualBlockHandle
	{
		VmaVirtualBlock block{nullptr};

		VmaVirtualBlockCreateInfo virtual_block_create_info{};
		virtual_block_create_info.size = p_size;

		VkResult result{vmaCreateVirtualBlock(&virtual_block_create_info, &block)};
		if (result != VK_SUCCESS)
			return nullptr;

		return p_ctx->virtualBlocks.emplace(VirtualBlock{block});
	}

	auto virtual_block_destroy(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block) -> void
	{
		p_ctx->virtualBlocks.destroy(p_virtual_block);
	}

	auto virtual_block_allocate(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block, uint64 p_size, uint64 p_alignment) -> VirtualAllocationHandle
	{
		VirtualBlock &block{p_ctx->virtualBlocks[p_virtual_block]};

		VmaVirtualAllocation allocation{nullptr};

		uint64 allocation_offset{0u};

		VmaVirtualAllocationCreateInfo virtual_allocation_create_info{};
		virtual_allocation_create_info.size      = p_size;
		virtual_allocation_create_info.alignment = p_alignment;

		VkResult result{vmaVirtualAllocate(block.block, &virtual_allocation_create_info, &allocation, &allocation_offset)};
		if (result != VK_SUCCESS)
			return nullptr;

		return p_ctx->virtualAllocations.emplace(VirtualAllocation{allocation, p_virtual_block, allocation_offset});
	}

	auto virtual_block_free(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> void
	{
		p_ctx->virtualAllocations.destroy(p_virtual_allocation);
	}

	auto virtual_allocation_get_offset(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64
	{
		VirtualAllocation &allocation{p_ctx->virtualAllocations[p_virtual_allocation]};
		return allocation.offset;
	}

	auto virtual_allocation_get_size(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64
	{
		VirtualAllocation &allocation{p_ctx->virtualAllocations[p_virtual_allocation]};
		return allocation.size;
	}

	auto page_allocator_create(AllocationContext *p_ctx, PageAllocator *p_out_allocator, uint64 p_page_size, ResourceDescriptorHeapHandle p_resource_heap) -> void
	{
		TST_ASSERT(p_ctx->gpuCtx);

		p_out_allocator->resourceHeap = p_resource_heap;
		p_out_allocator->pageSize     = p_page_size;
		_page_allocator_insert_page(p_ctx, p_out_allocator); // Insert the default page
	}

	auto page_allocator_destroy(AllocationContext *p_ctx, PageAllocator *p_allocator) -> void
	{
		TST_ASSERT(p_ctx->gpuCtx);

		for (auto &page: p_allocator->pages | std::views::values)
			_page_allocator_destroy_page(p_ctx, p_allocator, &page);
		p_allocator->pages.clear();

		p_allocator->resourceHeap = nullptr;
		p_allocator->pageSize     = 0u;
		p_allocator->nextPageId   = 0u;
		p_allocator->activePageId = 0u;
	}

	auto page_allocator_free_page_allocation(AllocationContext *p_ctx, PageAllocator *p_allocator, PageAllocation *p_allocation) -> void
	{
		TST_ASSERT(p_ctx->gpuCtx);

		auto page_it{p_allocator->pages.find(p_allocation->pageId)};
		if (page_it != p_allocator->pages.end())
		{
			virtual_block_free(p_ctx, p_allocation->allocation);
			TST_SCOPED_ATOMIC(page_it->second.activeAllocations, active_allocations);
			--active_allocations;

			if (active_allocations.load() == 0u && page_it->first != p_allocator->activePageId)
			{
				_page_allocator_destroy_page(p_ctx, p_allocator, &page_it->second);
				p_allocator->pages.erase(page_it);
			}
		}
		p_allocation->buffer     = nullptr;
		p_allocation->allocation = nullptr;
		p_allocation->offset     = 0u;
		p_allocation->pageId     = 0u;
		p_allocation->heapSlot   = UINT32_MAX;
	}

	auto page_allocator_allocate_across_pages(AllocationContext *p_ctx, PageAllocator *p_allocator, uint64 p_size, uint64 p_alignment,
											  PageAllocation *   p_out_allocation) -> void
	{
		TST_ASSERT(p_ctx->gpuCtx);

		if (_page_allocator_try_allocate(p_ctx, p_allocator, &p_allocator->pages[p_allocator->activePageId], p_size, p_alignment, p_out_allocation))
			return;

		for (auto &page: p_allocator->pages | std::views::values)
		{
			if (_page_allocator_try_allocate(p_ctx, p_allocator, &page, p_size, p_alignment, p_out_allocation))
				return;
		}

		if (p_size > p_allocator->pageSize) // Create a dedicated allocation
		{
			TST_SCOPED_ATOMIC(p_allocator->nextPageId, next_page_id);
			uint32 page_id{++next_page_id};
			p_allocator->pages[page_id] = _page_allocator_create_page(p_ctx, p_allocator, p_size, page_id);
			if (!_page_allocator_try_allocate(p_ctx, p_allocator, &p_allocator->pages[page_id], p_size, p_alignment, p_out_allocation))
				TST_PERMA_ASSERT(false);
			return;
		}

		_page_allocator_insert_page(p_ctx, p_allocator);
		if (!_page_allocator_try_allocate(p_ctx, p_allocator, &p_allocator->pages[p_allocator->activePageId], p_size, p_alignment, p_out_allocation))
		{
			TST_PERMA_ASSERT(false);
		}
	}

	auto _page_allocator_insert_page(AllocationContext *p_ctx, PageAllocator *p_allocator) -> void
	{
		TST_SCOPED_ATOMIC(p_allocator->nextPageId, next_page_id);
		uint32 page_id{++next_page_id};
		p_allocator->pages[page_id] = _page_allocator_create_page(p_ctx, p_allocator, p_allocator->pageSize, page_id);
		p_allocator->activePageId   = page_id;
	}

	auto _page_allocator_try_allocate(AllocationContext *p_ctx, PageAllocator *p_allocator, Page *p_page, uint64 p_size, uint64 p_alignment,
									  PageAllocation *   p_out_allocation) -> bool
	{
		p_out_allocation->allocation = virtual_block_allocate(p_ctx, p_page->block, p_size, p_alignment);
		if (p_out_allocation->allocation)
		{
			p_out_allocation->buffer   = p_page->buffer;
			p_out_allocation->offset   = virtual_allocation_get_offset(p_ctx, p_out_allocation->allocation);
			p_out_allocation->pageId   = p_page->id;
			p_out_allocation->heapSlot = p_page->heapSlot;

			TST_SCOPED_ATOMIC(p_page->activeAllocations, active_allocations);
			++active_allocations;

			return true;
		}
		return false;
	}

	auto _page_allocator_create_page(AllocationContext *p_ctx, PageAllocator *p_allocator, uint64 p_size, uint32 p_id) -> Page
	{
		Page page{};
		page.size = p_size;
		page.id   = p_id;

		BufferDesc page_desc{};
		page_desc.size       = p_size;
		page_desc.memoryType = EMemoryType::eDeviceLocal;
		page_desc.usage      = EBufferUsageFlagBits::eTransferDst | EBufferUsageFlagBits::eStorageBuffer;
		page.buffer          = buffer_create(p_ctx->gpuCtx, page_desc);

		page.block = virtual_block_create(p_ctx, p_size);

		page.heapSlot = resource_descriptor_heap_alloc_buffer_slot(p_ctx->gpuCtx, p_allocator->resourceHeap);
		resource_descriptor_heap_write_buffer(p_ctx->gpuCtx, p_allocator->resourceHeap, page.heapSlot, page.buffer);

		return page;
	}

	auto _page_allocator_destroy_page(AllocationContext *p_ctx, PageAllocator *p_allocator, Page *p_page) -> void
	{
		virtual_block_destroy(p_ctx, p_page->block);
		buffer_destroy(p_ctx->gpuCtx, p_page->buffer);
		resource_descriptor_heap_free_buffer_slot(p_ctx->gpuCtx, p_allocator->resourceHeap, p_page->heapSlot);
	}
}
