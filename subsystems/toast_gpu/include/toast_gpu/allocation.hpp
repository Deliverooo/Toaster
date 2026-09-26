#pragma once

#include <mutex>
#include <unordered_map>

#include "api.hpp"
#include "toast_lib/atomic.hpp"

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

	class TST_GPU_API GPUPageAllocator
	{
	public:
		GPUPageAllocator(uint64 p_page_size, ResourceDescriptorHeapHandle p_resource_heap);
		~GPUPageAllocator();

		auto insertPage() -> void;
		auto freePageAllocation(const PageAllocation &p_allocation) -> void;
		auto allocateAcrossPages(uint64 p_size, uint64 p_alignment, PageAllocation &p_out_allocation) -> void;

	private:
		static auto tryAllocate(Page &p_page, uint64 p_size, uint64 p_alignment, PageAllocation &p_out_allocation) -> bool;
		auto createPage(uint64 p_size, uint32 p_id) -> Page;
		auto destroyPage(Page &p_page) -> void;

		std::unordered_map<uint32, Page> pages;
		std::mutex                       pageMutex;

		ResourceDescriptorHeapHandle m_resourceHeap{nullptr};

		uint64 pageSize{0u};
		uint32 nextPageId{0u};
		uint32 activePageId{0u};
	};
}
