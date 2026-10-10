#pragma once

#include <functional>
#include <mutex>

#include "frame.hpp"

namespace toaster::gpu::upload
{
	struct TST_GPU_API UploadContextDesc
	{
		uint64 maxStagingSize{1024u * 1024u * 500u}; // 500 Mib

		uint32 maxAllocationCommandLists{2u}; // Basically frames in flight, but for the transfer queue
	};

	// Must be called after initFrameContext(...)
	auto TST_GPU_API initUploadContext(const UploadContextDesc &p_desc) -> void;
	// Must be called before shutdownFrameContext()
	auto TST_GPU_API shutdownUploadContext() -> void;

	#pragma region state tracker

	TST_DECLARE_GPU_HANDLE(StateTracker);

	using StateTrackerReadyFn = std::function<void()>; // Std function is okay to use because it will only be invoked once

	// State trackers are automatically destroyed upon completion...
	auto TST_GPU_API registerStateTracker(uint32 p_expected_subresources) -> StateTrackerHandle;

	// Invokes when the state tracker has finished processing all the associated subresources
	auto TST_GPU_API registerStateTrackerReadyCallback(StateTrackerHandle p_state_tracker, StateTrackerReadyFn p_ready_callback) -> void;
	auto TST_GPU_API isStateTrackerReady(StateTrackerHandle p_state_tracker) -> bool;

	auto TST_GPU_API waitForStateTracker(StateTrackerHandle p_state_tracker) -> void; // Blocks until all pending subresources have completed execution

	#pragma endregion

	auto TST_GPU_API pollUploads() -> void;

	struct TST_GPU_API BufferUploadDesc
	{
		BufferHandle dstBuffer{nullptr};
		const void * data{nullptr};
		uint64       size{0u};
		uint64       dstOffset{0u};
	};

	auto TST_GPU_API uploadDataToBuffer(const BufferUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void;

	struct TST_GPU_API TextureUploadDesc
	{
		TextureHandle dstTexture{nullptr};
		const void *  data{nullptr};
		uint64        size{0u};
		tsm::uint3    extent{0u}; // If zero, uses the texture's actual size
		tsm::int3     offset{0};
		uint32        mipLevel{0u};
		uint32        baseLayer{0u};
		uint32        layerCount{1u};
	};

	auto TST_GPU_API uploadDataToTexture(const TextureUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void;
}
