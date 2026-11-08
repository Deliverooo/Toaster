#include "toast_gpu/allocation.hpp"
#include "toast_lib/pool.hpp"

#include <vk_mem_alloc.h>

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

		uint64 offset{0ull}; // Offset at which this allocation was made into the virtual block
		uint64 size{0ull};   // Size of the allocation
	};

	struct AllocationContext
	{
		FixedPool<VirtualBlock>      virtualBlocks;
		FixedPool<VirtualAllocation> virtualAllocations;
	};

	auto allocation_ctx_create() -> AllocationContext *
	{
		const auto ctx{new AllocationContext{}};

		ctx->virtualBlocks.setDestructorFn(+[](VirtualBlock *p_data, void *) -> void
		{
			vmaDestroyVirtualBlock(p_data->block);
		});

		ctx->virtualAllocations.setDestructorUserData(ctx);
		ctx->virtualAllocations.setDestructorFn(+[](VirtualAllocation *p_data, void *p_ctx) -> void
		{
			const auto ctx{static_cast<AllocationContext *>(p_ctx)};

			#ifndef NDEBUG
			if (const VirtualBlock *block{ctx->virtualBlocks.try_get(p_data->block)})
				vmaVirtualFree(block->block, p_data->allocation);
			else
				TST_PERMA_ASSERT_MSG(false, "Attempted to free virtual allocation after destroying associated block!");
			#else
			const VirtualBlock &block{ctx->virtualBlocks[p_data->block]}; vmaVirtualFree(block.block, p_data->allocation);
			#endif
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

		if (vmaCreateVirtualBlock(&virtual_block_create_info, &block) != VK_SUCCESS)
			return nullptr;

		return p_ctx->virtualBlocks.emplace(VirtualBlock{block});
	}

	auto virtual_block_destroy(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block) -> void
	{
		p_ctx->virtualBlocks.destroy(p_virtual_block);
	}

	auto virtual_block_allocate(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block, uint64 p_size, uint64 p_alignment) -> VirtualAllocationHandle
	{
		const VirtualBlock &block{p_ctx->virtualBlocks[p_virtual_block]};

		VmaVirtualAllocationCreateInfo virtual_allocation_create_info{};
		virtual_allocation_create_info.size      = p_size;
		virtual_allocation_create_info.alignment = p_alignment;

		VmaVirtualAllocation allocation;
		uint64               allocation_offset;

		if (vmaVirtualAllocate(block.block, &virtual_allocation_create_info, &allocation, &allocation_offset) != VK_SUCCESS)
			return nullptr;

		return p_ctx->virtualAllocations.emplace(VirtualAllocation{allocation, p_virtual_block, allocation_offset});
	}

	auto virtual_block_free(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> void
	{
		p_ctx->virtualAllocations.destroy(p_virtual_allocation);
	}

	auto virtual_allocation_get_offset(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64
	{
		return p_ctx->virtualAllocations[p_virtual_allocation].offset;
	}

	auto virtual_allocation_get_size(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64
	{
		return p_ctx->virtualAllocations[p_virtual_allocation].size;
	}
}
