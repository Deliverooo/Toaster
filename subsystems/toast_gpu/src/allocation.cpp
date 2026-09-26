#include "toast_gpu/allocation.hpp"
#include "toast_lib/pool.hpp"

#include <vma/vk_mem_alloc.h>

namespace toaster::gpu::alloc
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

	struct AllocationContextImpl
	{
		Pool<VirtualBlock>      virtualBlocks;
		Pool<VirtualAllocation> virtualAllocations;
	};

	static AllocationContextImpl *g_impl{nullptr};

	auto initAllocationContext() -> void
	{
		if (g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Frame context has already been initialised!");
			return;
		}

		g_impl = new AllocationContextImpl{};

		g_impl->virtualBlocks.setDestructorFn(+[](VirtualBlock *p_data, void *) -> void
		{
			vmaDestroyVirtualBlock(p_data->block);
		});

		g_impl->virtualAllocations.setDestructorFn(+[](VirtualAllocation *p_data, void *) -> void
		{
			if (const VirtualBlock *block{g_impl->virtualBlocks.tryGet(p_data->block)})
				vmaVirtualFree(block->block, p_data->allocation);
			else
			{
				TST_PERMA_ASSERT_MSG(false, "Attempted to free virtual allocation after destroying associated block!");
			}
		});
	}

	auto shutdownAllocationContext() -> void
	{
		if (!g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Frame context has not been initialised!");
			return;
		}
		g_impl->virtualAllocations.clear();
		g_impl->virtualBlocks.clear();

		delete g_impl;
		g_impl = nullptr;
	}

	auto createVirtualBlock(uint64 p_size) -> VirtualBlockHandle
	{
		VmaVirtualBlock block{nullptr};

		VmaVirtualBlockCreateInfo virtual_block_create_info{};
		virtual_block_create_info.size = p_size;

		VkResult result{vmaCreateVirtualBlock(&virtual_block_create_info, &block)};
		if (result != VK_SUCCESS)
			return nullptr;

		return g_impl->virtualBlocks.emplace(VirtualBlock{block});
	}

	auto destroyVirtualBlock(VirtualBlockHandle p_virtual_block) -> void
	{
		g_impl->virtualBlocks.destroy(p_virtual_block);
	}

	auto virtualAllocate(VirtualBlockHandle p_virtual_block, uint64 p_size, uint64 p_alignment) -> VirtualAllocationHandle
	{
		VirtualBlock &block{g_impl->virtualBlocks[p_virtual_block]};

		VmaVirtualAllocation allocation{nullptr};

		uint64 allocation_offset{0u};

		VmaVirtualAllocationCreateInfo virtual_allocation_create_info{};
		virtual_allocation_create_info.size      = p_size;
		virtual_allocation_create_info.alignment = p_alignment;

		VkResult result{vmaVirtualAllocate(block.block, &virtual_allocation_create_info, &allocation, &allocation_offset)};
		if (result != VK_SUCCESS)
			return nullptr;

		return g_impl->virtualAllocations.emplace(VirtualAllocation{allocation, p_virtual_block, allocation_offset});
	}

	auto virtualFree(VirtualAllocationHandle p_virtual_allocation) -> void
	{
		g_impl->virtualAllocations.destroy(p_virtual_allocation);
	}

	auto getAllocationOffset(VirtualAllocationHandle p_virtual_allocation) -> uint64
	{
		VirtualAllocation &allocation{g_impl->virtualAllocations[p_virtual_allocation]};
		return allocation.offset;
	}

	auto getAllocationSize(VirtualAllocationHandle p_virtual_allocation) -> uint64
	{
		VirtualAllocation &allocation{g_impl->virtualAllocations[p_virtual_allocation]};
		return allocation.size;
	}

	GPUPageAllocator::GPUPageAllocator(uint64 p_page_size, ResourceDescriptorHeapHandle p_resource_heap) : m_resourceHeap(p_resource_heap), pageSize(p_page_size)
	{
		insertPage(); // Insert the default page
	}

	GPUPageAllocator::~GPUPageAllocator()
	{
		for (auto &page: pages | std::views::values)
			destroyPage(page);
	}

	auto GPUPageAllocator::insertPage() -> void
	{
		uint32 page_id{++nextPageId};
		pages[page_id] = createPage(pageSize, page_id);
		activePageId   = page_id;
	}

	auto GPUPageAllocator::freePageAllocation(const PageAllocation &p_allocation) -> void
	{
		auto page_it{pages.find(p_allocation.pageId)};
		if (page_it != pages.end())
		{
			virtualFree(p_allocation.allocation);
			TST_SCOPED_ATOMIC(page_it->second.activeAllocations, active_allocations);
			--active_allocations;

			if (active_allocations.load() == 0u && page_it->first != activePageId)
			{
				destroyPage(page_it->second);
				pages.erase(page_it);
			}
		}
	}

	auto GPUPageAllocator::allocateAcrossPages(uint64 p_size, uint64 p_alignment, PageAllocation &p_out_allocation) -> void
	{
		std::scoped_lock<std::mutex> page_lock{pageMutex};

		if (tryAllocate(pages[activePageId], p_size, p_alignment, p_out_allocation))
			return;

		for (auto &[page_id,page]: pages)
		{
			if (tryAllocate(page, p_size, p_alignment, p_out_allocation))
				return;
		}

		if (p_size > pageSize)
		{
			uint32 page_id{++nextPageId};
			pages[page_id] = createPage(p_size, page_id);
		}

		insertPage();
		if (!tryAllocate(pages[activePageId], p_size, p_alignment, p_out_allocation))
		{
			TST_PERMA_ASSERT(false);
		}
	}

	auto GPUPageAllocator::tryAllocate(Page &p_page, uint64 p_size, uint64 p_alignment, PageAllocation &p_out_allocation) -> bool
	{
		p_out_allocation.allocation = virtualAllocate(p_page.block, p_size, p_alignment);
		if (p_out_allocation.allocation)
		{
			p_out_allocation.buffer   = p_page.buffer;
			p_out_allocation.offset   = getAllocationOffset(p_out_allocation.allocation);
			p_out_allocation.pageId   = p_page.id;
			p_out_allocation.heapSlot = p_page.heapSlot;

			TST_SCOPED_ATOMIC(p_page.activeAllocations, active_allocations);
			++active_allocations;

			return true;
		}
		return false;
	}

	auto GPUPageAllocator::createPage(uint64 p_size, uint32 p_id) -> Page
	{
		Page page{};
		page.size = p_size;
		page.id   = p_id;

		BufferDesc page_desc{};
		page_desc.size       = p_size;
		page_desc.memoryType = EMemoryType::eDeviceLocal;
		page_desc.usage      = EBufferUsageFlagBits::eTransferDst | EBufferUsageFlagBits::eStorageBuffer;
		page.buffer          = createBuffer(page_desc);

		page.block = createVirtualBlock(p_size);

		page.heapSlot = allocBufferHeapSlot(m_resourceHeap);
		writeBufferDescriptor(m_resourceHeap, page.heapSlot, page.buffer);

		return page;
	}

	auto GPUPageAllocator::destroyPage(Page &p_page) -> void
	{
		destroyVirtualBlock(p_page.block);
		destroyBuffer(p_page.buffer);
		freeBufferHeapSlot(m_resourceHeap, p_page.heapSlot);
	}
}
