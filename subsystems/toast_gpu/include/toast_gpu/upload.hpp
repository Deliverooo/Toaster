#pragma once

#include <functional>
#include "fwd.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::gpu
{
	struct GPUContext;
	struct AllocationContext;
	struct UploadContext;

	struct TST_GPU_API UploadContextDesc
	{
		uint64 maxStagingSize{1024u * 1024u * 500u}; // 500 Mib

		uint32 maxAllocationCommandLists{2u}; // Basically frames in flight, but for the transfer queue
	};

	TST_GPU_DECLARE_GPU_HANDLE(StateTracker);
	using StateTrackerReadyFn = std::function<void()>; // Std function is okay to use because it will only be invoked once

	struct TST_GPU_API BufferUploadDesc
	{
		BufferHandle dstBuffer{nullptr};
		const void * data{nullptr};
		uint64       size{0u};
		uint64       dstOffset{0u};
	};

	struct TST_GPU_API TextureUploadDesc
	{
		TextureHandle dstTexture{nullptr};
		const void *  data{nullptr};
		uint64        size{0u};
		math::UInt3   extent{0u, 0u, 0u}; // If zero, uses the texture's actual size
		math::Int3    offset{0, 0, 0};
		uint32        mipLevel{0u};
		uint32        baseLayer{0u};
		uint32        layerCount{1u};
	};

	// Must be called after gpu_ctx_create(...)
	auto TST_GPU_API upload_ctx_create(GPUContext *p_gpu_ctx, AllocationContext *p_alloc_ctx, const UploadContextDesc &p_desc) -> UploadContext *;
	// Must be called before gpu_ctx_destroy()
	auto TST_GPU_API upload_ctx_destroy(UploadContext *p_ctx) -> void;

	// State trackers are automatically destroyed upon completion...
	auto TST_GPU_API state_tracker_register(UploadContext *p_ctx, uint32 p_expected_subresources) -> StateTrackerHandle;

	// Invokes when the state tracker has finished processing all the associated subresources
	auto TST_GPU_API state_tracker_register_ready_callback(UploadContext *p_ctx, StateTrackerHandle p_state_tracker, StateTrackerReadyFn p_ready_callback) -> void;
	auto TST_GPU_API state_tracker_is_ready(UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> bool;
	auto TST_GPU_API state_tracker_wait_for(UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> void;
	// Blocks until all pending subresources have completed execution

	auto TST_GPU_API poll_uploads(UploadContext *p_ctx) -> void;

	auto TST_GPU_API upload_data_to_buffer(UploadContext *p_ctx, const BufferUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void;
	auto TST_GPU_API upload_data_to_texture(UploadContext *p_ctx, const TextureUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void;

	// You should not really be calling these, but here...
	[[nodiscard]] auto TST_GPU_API _get_transfer_timeline_semaphore(UploadContext *p_ctx) -> SemaphoreHandle;
	[[nodiscard]] auto TST_GPU_API _acquire_transfer_timeline_counter_value(UploadContext *p_ctx) -> uint64; // returns the current value, then increments the counter
	[[nodiscard]] auto TST_GPU_API _get_transfer_timeline_counter_value(UploadContext *p_ctx) -> uint64;
}
