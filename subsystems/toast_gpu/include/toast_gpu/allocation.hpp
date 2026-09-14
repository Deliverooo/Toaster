#pragma once

#include "toast_gpu.hpp"

// This whole file is essentially just a wrapper for VMA's virtual allocation system
namespace toaster::gpu::alloc
{
	TST_DECLARE_GPU_HANDLE(VirtualBlock);
	TST_DECLARE_GPU_HANDLE(VirtualAllocation);

	auto TST_GPU_API initAllocationContext() -> void;
	auto TST_GPU_API shutdownAllocationContext() -> void;

	auto TST_GPU_API createVirtualBlock(uint64 p_size) -> VirtualBlockHandle;
	auto TST_GPU_API destroyVirtualBlock(VirtualBlockHandle p_virtual_block) -> void;

	auto TST_GPU_API virtualAllocate(VirtualBlockHandle p_virtual_block, uint64 p_size, uint64 p_alignment) -> VirtualAllocationHandle;
	auto TST_GPU_API virtualFree(VirtualAllocationHandle p_virtual_allocation) -> void;

	auto TST_GPU_API getAllocationOffset(VirtualAllocationHandle p_virtual_allocation) -> uint64;
	auto TST_GPU_API getAllocationSize(VirtualAllocationHandle p_virtual_allocation) -> uint64;
}
