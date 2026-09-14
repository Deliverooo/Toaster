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

		vmaCreateVirtualBlock(&virtual_block_create_info, &block);
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
		vmaVirtualAllocate(block.block, &virtual_allocation_create_info, &allocation, &allocation_offset);

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
}
