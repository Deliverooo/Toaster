#pragma once

#include "fwd.hpp"

// This whole file is essentially just a wrapper for VMA's virtual allocation system
namespace toaster::gpu
{
	auto TST_GPU_API allocation_ctx_create() -> AllocationContext *;
	auto TST_GPU_API allocation_ctx_destroy(AllocationContext *p_ctx) -> void;

	auto TST_GPU_API virtual_block_create(AllocationContext *p_ctx, uint64 p_size) -> VirtualBlockHandle;
	auto TST_GPU_API virtual_block_destroy(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block) -> void;

	auto TST_GPU_API virtual_block_allocate(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block, uint64 p_size, uint64 p_alignment) -> VirtualAllocationHandle;
	auto TST_GPU_API virtual_block_free(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> void;

	auto TST_GPU_API virtual_allocation_get_offset(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64;
	auto TST_GPU_API virtual_allocation_get_size(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64;
}
