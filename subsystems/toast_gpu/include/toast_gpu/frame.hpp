#pragma once

#include "fwd.hpp"

namespace toaster::gpu
{
	struct GPUContext;
	struct FrameContext;

	struct TST_GPU_API FrameContextDesc
	{
		uint32 maxFramesInFlight{3u}; // Same as GPUContextDesc::maxConcurrentSwapchainWorkloads...
	};

	// Must be called after gpu_ctx_create(...)
	auto TST_GPU_API frame_ctx_create(GPUContext *p_gpu_ctx, const FrameContextDesc &p_desc) -> FrameContext *;
	// Must be called before gpu_ctx_destroy()
	auto TST_GPU_API frame_ctx_destroy(FrameContext *p_ctx) -> void;

	auto TST_GPU_API frame_ctx_get_max_frames_in_flight(FrameContext *p_ctx) -> uint32; // You should be tracking this yourself, but here.

	// Waits on timeline semaphores and resets previous command lists
	auto TST_GPU_API begin_frame(FrameContext *p_ctx, uint32 p_frame_index) -> void;
	auto TST_GPU_API process_deferred_deletions(FrameContext *p_ctx) -> void; // Don't call this unless you have to, as it is already invoked in begin_frame()

	// Returns true if successful, false if the present operation was unsuccessful. Recreate if false
	// Prefer over the standard on from <api.hpp> because this has automatic timeline semaphore tracking
	auto TST_GPU_API submit_and_present(FrameContext *p_ctx, SwapchainHandle p_swapchain, CommandListHandle p_command_list) -> bool;

	auto TST_GPU_API get_graphics_timeline_semaphore_current_value(FrameContext *p_ctx) -> uint64;

	auto TST_GPU_API defer_buffer_deletion(FrameContext *p_ctx, BufferHandle p_buffer) -> void;
	auto TST_GPU_API defer_texture_deletion(FrameContext *p_ctx, TextureHandle p_texture) -> void;

	auto TST_GPU_API defer_buffer_heap_slot_freeing(FrameContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_buffer_slot) -> void;
	auto TST_GPU_API defer_texture_heap_slot_freeing(FrameContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_texture_slot) -> void;
	auto TST_GPU_API defer_sampler_Heap_slot_freeing(FrameContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_sampler_slot) -> void;
}
