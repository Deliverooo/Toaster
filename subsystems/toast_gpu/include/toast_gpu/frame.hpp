#pragma once

#include "api.hpp"

#include "toast_lib/command_buffer.hpp"

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

	auto TST_GPU_API processDeferredDeletions() -> void; // Don't call this unless you have to, as it is already invoked in beginFrame()

	// Adds a command list to the queue of command lists that will be reset when begin frame is called and the semaphores are waited on
	auto TST_GPU_API addCommandListToBeReset(CommandListHandle p_command_list) -> void;

	// Returns true if successful, false if the present operation was unsuccessful. Recreate if false
	// Prefer over the standard on from <api.hpp> because this has automatic timeline semaphore tracking
	auto TST_GPU_API submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list) -> bool;

	[[nodiscard]] auto TST_GPU_API getGraphicsTimelineSemaphoreCurrentValue() -> uint64;

	auto TST_GPU_API deferBufferDeletion(BufferHandle p_buffer) -> void;
	auto TST_GPU_API deferTextureDeletion(TextureHandle p_texture) -> void;

	auto TST_GPU_API deferBufferHeapSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_buffer_slot) -> void;
	auto TST_GPU_API deferTextureHeapSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_texture_slot) -> void;
	auto TST_GPU_API deferSamplerHeapSlotFreeing(SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_sampler_slot) -> void;

	template<typename TFunc>
	auto TST_GPU_API deferDeletion(TFunc &&p_destructor) -> void
	{
	}
}
