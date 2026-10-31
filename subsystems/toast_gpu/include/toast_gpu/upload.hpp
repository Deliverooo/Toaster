#pragma once

#include <functional>
#include <span>

#include "api.hpp"
#include "fwd.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::gpu
{
	struct TST_GPU_API UploadContextDesc
	{
		uint32 maxAllocationCommandLists{2u}; // Basically frames in flight, but for the transfer queue
	};

	using StateTrackerReadyFn = std::function<void()>; // Std function is okay to use because it will only be invoked once

	struct TST_GPU_API BufferUploadDesc
	{
		const void *data{nullptr};
		uint64      size{0u};
		uint64      dstOffset{0u};
	};

	struct TST_GPU_API TextureUploadDesc
	{
		const void *data{nullptr};
		uint64      size{0u};

		// The buffer offset in here must be < size
		InitialiserList<const BufferTextureCopyRegion> uploadRegions;
	};

	// Must be called after gpu_ctx_create(...)
	auto TST_GPU_API upload_ctx_create(GPUContext *p_gpu_ctx, AllocationContext *p_alloc_ctx, const UploadContextDesc &p_desc) -> UploadContext *;
	// Must be called before gpu_ctx_destroy()
	auto TST_GPU_API upload_ctx_destroy(UploadContext *p_ctx) -> void;

	// State trackers are automatically destroyed upon completion.
	// p_expected_uploads are the number of transfer operations that need to complete before the state tracker is destroyed
	auto TST_GPU_API state_tracker_register(UploadContext *p_ctx, uint32 p_expected_uploads) -> StateTrackerHandle;

	// Invokes when the state tracker has finished processing all the associated uploads
	auto TST_GPU_API state_tracker_register_ready_callback(UploadContext *p_ctx, StateTrackerHandle p_state_tracker, StateTrackerReadyFn p_ready_callback) -> void;
	// If the state tracker has been destroyed, then it is considered 'ready'
	auto TST_GPU_API state_tracker_is_ready(const UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> bool;
	// Blocks until all pending uploads have completed execution
	auto TST_GPU_API state_tracker_wait_for(UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> void;

	auto TST_GPU_API poll_uploads(UploadContext *p_ctx) -> void; // Destroys any completed state trackers and cleans up staging memory if needed

	auto TST_GPU_API upload_data_to_buffer(UploadContext *    p_ctx, BufferHandle p_dst_buffer, const BufferUploadDesc &p_upload_desc,
										   StateTrackerHandle p_state_tracker) -> void;
	auto TST_GPU_API upload_data_to_texture(UploadContext *    p_ctx, TextureHandle p_dst_texture, const TextureUploadDesc &p_upload_desc,
											StateTrackerHandle p_state_tracker) -> void;

	// You should not really be calling these, but here...
	[[nodiscard]] auto TST_GPU_API _get_transfer_timeline_semaphore(const UploadContext *p_ctx) -> SemaphoreHandle;
	[[nodiscard]] auto TST_GPU_API _acquire_transfer_timeline_counter_value(UploadContext *p_ctx) -> uint64; // returns the current value, then increments the counter
	[[nodiscard]] auto TST_GPU_API _get_transfer_timeline_counter_value(const UploadContext *p_ctx) -> uint64;
}
