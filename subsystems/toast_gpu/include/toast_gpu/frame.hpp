#pragma once

#include "api.hpp"

namespace toaster::gpu::frame
{
	struct TST_GPU_API FrameContextDesc
	{
		uint32 maxFramesInFlight{3u}; // Same as GPUContextDesc::maxConcurrentSwapchainWorkloads...
	};

	// Must be called after initGPUContext(...)
	auto TST_GPU_API initFrameContext(const FrameContextDesc &p_desc) -> void;
	// Must be called before shutdownGPUContext()
	auto TST_GPU_API shutdownFrameContext() -> void;

	auto TST_GPU_API getMaxFramesInFlight() -> uint32; // You should be tracking this yourself, but here.

	// Waits on timeline semaphores and resets previous command lists
	auto TST_GPU_API beginFrame(uint32 p_frame_index) -> void;

	// Adds a command list to the queue of command lists that will be reset when begin frame is called and the semaphores are waited on
	auto TST_GPU_API addCommandListToBeReset(CommandListHandle p_command_list) -> void;

	// Returns true if successful, false if the present operation was unsuccessful. Recreate if false
	// Prefer over the standard on from <api.hpp> because this has automatic timeline semaphore tracking
	auto TST_GPU_API submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list) -> bool;

	// Uploads signal this timeline; graphics submissions wait on its latest value.
	[[nodiscard]] auto TST_GPU_API getTransferTimelineSemaphore() -> SemaphoreHandle;
	[[nodiscard]] auto TST_GPU_API acquireTransferTimelineCounterValue() -> uint64; // returns the current value, then increments the counter
	[[nodiscard]] auto TST_GPU_API getTransferTimelineCounterValue() -> uint64;

	auto TST_GPU_API defferBufferDeletion(BufferHandle p_buffer) -> void;
	auto TST_GPU_API defferTextureDeletion(TextureHandle p_texture) -> void;

	// Used so that the gpu doesn't ever reference an invalid heap slot
	auto TST_GPU_API defferBufferSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_slot) -> void;
	auto TST_GPU_API defferTextureSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_slot) -> void;

	// You won't need to deffer a sampler heap slot freeing
}
