#pragma once

#include <threads.h>

#include "fwd.hpp"
#include "toast_lib/atomic.hpp"
#include "toast_lib/freelist_allocator.hpp"

// This whole file is essentially just a wrapper for VMA's virtual allocation system
namespace toaster::gpu
{
	auto TST_GPU_API allocation_ctx_create(GPUContext *p_gpu_ctx) -> AllocationContext *; // If no GPU context is provided, you cannot use the PageAllocator
	auto TST_GPU_API allocation_ctx_destroy(AllocationContext *p_ctx) -> void;

	auto TST_GPU_API virtual_block_create(AllocationContext *p_ctx, uint64 p_size) -> VirtualBlockHandle;
	auto TST_GPU_API virtual_block_destroy(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block) -> void;

	auto TST_GPU_API virtual_block_allocate(AllocationContext *p_ctx, VirtualBlockHandle p_virtual_block, uint64 p_size, uint64 p_alignment) -> VirtualAllocationHandle;
	auto TST_GPU_API virtual_block_free(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> void;

	auto TST_GPU_API virtual_allocation_get_offset(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64;
	auto TST_GPU_API virtual_allocation_get_size(AllocationContext *p_ctx, VirtualAllocationHandle p_virtual_allocation) -> uint64;

	struct TST_GPU_API Page
	{
		BufferHandle             buffer{nullptr};
		VirtualBlockHandle       block{nullptr};
		uint64                   size{0u};
		uint32                   heapSlot{UINT32_MAX};
		uint32                   id{0u};
		TST_ALIGN_ATOMIC(uint32) activeAllocations{0u};
	};

	struct TST_GPU_API PageAllocation
	{
		BufferHandle            buffer{nullptr};
		VirtualAllocationHandle allocation{nullptr};
		uint64                  offset{0u};
		uint32                  pageId{0u};
		uint32                  heapSlot{UINT32_MAX}; // Reference to Page::heapSlot
	};

	struct TST_GPU_API PageAllocator
	{
		FreelistAllocator<uint32>    pageIdAllocator{};
		mtx_t                        freeListMtx;
		Vector<Page>                 pages;
		ResourceDescriptorHeapHandle resourceHeap{nullptr};
		uint64                       pageSize{0u};
		uint32                       activePageId{0u};
	};

	auto TST_GPU_API page_allocator_create(AllocationContext *          p_ctx, PageAllocator *p_out_allocator, uint64 p_page_size,
										   ResourceDescriptorHeapHandle p_resource_heap) -> void;
	auto TST_GPU_API page_allocator_destroy(AllocationContext *p_ctx, PageAllocator *p_allocator) -> void;

	auto TST_GPU_API page_allocator_free_page_allocation(AllocationContext *p_ctx, PageAllocator *p_allocator, PageAllocation *p_allocation) -> void;
	auto TST_GPU_API page_allocator_allocate_across_pages(AllocationContext *p_ctx, PageAllocator *p_allocator, uint64 p_size, uint64 p_alignment,
														  PageAllocation *   p_out_allocation) -> void;

	// Returns the total size of all allocated pages
	auto TST_GPU_API page_allocator_get_total_allocated_size(const AllocationContext *p_ctx, const PageAllocator *p_allocator) -> uint64;

	// Internals
	auto TST_GPU_API _page_allocator_insert_page(AllocationContext *p_ctx, PageAllocator *p_allocator) -> void;
	auto TST_GPU_API _page_allocator_try_allocate(AllocationContext *p_ctx, Page *p_page, uint64 p_size, uint64 p_alignment,
												  PageAllocation *   p_out_allocation) -> bool;
	auto TST_GPU_API _page_allocator_create_page(AllocationContext *p_ctx, const PageAllocator *p_allocator, uint64 p_size, uint32 p_id) -> Page;
	auto TST_GPU_API _page_allocator_destroy_page(AllocationContext *p_ctx, PageAllocator *p_allocator, Page *p_page) -> void;
}
