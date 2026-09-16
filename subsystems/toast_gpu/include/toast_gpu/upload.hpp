#pragma once

#include "frame.hpp"

namespace toaster::gpu::upload
{
	struct TST_GPU_API UploadContextDesc
	{
	};

	// Must be called after initFrameContext(...)
	auto TST_GPU_API initUploadContext(const UploadContextDesc &p_desc) -> void;
	// Must be called before shutdownFrameContext()
	auto TST_GPU_API shutdownUploadContext() -> void;

	auto TST_GPU_API flushUploads() -> void; // Only submits a transfer command buffer but does not wait on it!
	auto TST_GPU_API flushUploadsAndWait() -> void;

	// Returns the value to 'wait' on
	auto TST_GPU_API uploadDataToBuffer(BufferHandle p_dst_buffer, const void *p_data, uint64 p_size, uint64 p_offset = 0u) -> uint64;

	struct TST_GPU_API TextureUploadDesc
	{
		tsm::uint3 extent{0u}; // If zero, uses the texture's actual size
		uint32     mipLevel{0u};
		uint32     baseLayer{0u};
		uint32     layerCount{1u};
	};

	// Returns the value to 'wait' on
	auto TST_GPU_API uploadDataToTexture(TextureHandle p_dst_texture, const void *p_data, uint64 p_size, const TextureUploadDesc &p_desc) -> uint64;

	auto TST_GPU_API cancelBufferUpload(BufferHandle p_buffer) -> void;    // Only works if called before flushUploads!
	auto TST_GPU_API cancelTextureUpload(TextureHandle p_texture) -> void; // Only works if called before flushUploads!
}
