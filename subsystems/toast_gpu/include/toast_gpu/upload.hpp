#pragma once

#include <mutex>

#include "frame.hpp"

namespace toaster::gpu::upload
{
	struct TST_GPU_API UploadContextDesc
	{
		// TODO: Replace maybe with a multi-paged allocator, with per-allocation tracking. IYKYK
		uint64 maxStagingSize{1024u * 1024u * 10u}; // 10 Mib

		uint32 maxAllocationCommandLists{3u}; // Basically frames in flight, but for the transfer queue
	};

	// Must be called after initFrameContext(...)
	auto TST_GPU_API initUploadContext(const UploadContextDesc &p_desc) -> void;
	// Must be called before shutdownFrameContext()
	auto TST_GPU_API shutdownUploadContext() -> void;

	struct StateTracker
	{
		std::mutex           ticketMutex;
		std::vector<uint64>  timelineTickets;
		std::atomic_uint32_t pendingSubresources{0u};
		std::atomic_bool     ready{false};
	};

	auto TST_GPU_API pollUploads() -> void;

	struct TST_GPU_API BufferUploadDesc
	{
		BufferHandle dstBuffer{nullptr};
		const void * data{nullptr};
		uint64       size{0u};
		uint64       dstOffset{0u};
	};

	auto TST_GPU_API uploadDataToBuffer(const BufferUploadDesc &p_upload_desc, RefPtr<StateTracker> &p_state_tracker) -> void;

	struct TST_GPU_API TextureUploadDesc
	{
		TextureHandle dstTexture{nullptr};
		const void *  data{nullptr};
		uint64        size{0u};
		tsm::uint3    extent{0u}; // If zero, uses the texture's actual size
		uint32        mipLevel{0u};
		uint32        baseLayer{0u};
		uint32        layerCount{1u};
	};

	// Returns the value to 'wait' on
	auto TST_GPU_API uploadDataToTexture(const TextureUploadDesc &p_upload_desc, RefPtr<StateTracker> &p_state_tracker) -> void;

	// auto TST_GPU_API cancelBufferUpload(BufferHandle p_buffer) -> void;    // Only works if called before flushUploads!
	// auto TST_GPU_API cancelTextureUpload(TextureHandle p_texture) -> void; // Only works if called before flushUploads!
};

// class TST_GPU_API UploadManager
// {
// public:
// 	UploadContextDesc
//
// private:
//
// };
