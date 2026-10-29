#pragma once

#include "toast_render.hpp"
#include "toast_gpu/fwd.hpp"

namespace toaster::gpu
{
	struct TextureDesc;
}

namespace toaster::rnd
{
	struct RenderContext;

	constexpr uint32 c_maxTextureMipHeapSlots{16u}; // 2^16 = 65536... I don't think you will ever have a texture bigger than this...

	struct TST_RENDER_API Texture
	{
		gpu::TextureHandle      texture{nullptr};
		gpu::StateTrackerHandle stateTracker{nullptr};

		uint32                                  shaderReadHeapSlot{UINT32_MAX};
		uint32                                  storageHeapSlot{UINT32_MAX};
		Array<uint32, c_maxTextureMipHeapSlots> perMipStorageHeapSlots;
	};

	TST_DECLARE_HANDLE(Texture);

	constexpr uint32 textureSubresourceNoUpload{0u};      // No state tracker will be created
	constexpr uint32 textureSubresourceOneTimeUpload{1u}; // Set the expected subresouces for the state tracker to this for common use

	struct TextureManager;

	auto TST_RENDER_API texture_mngr_create(RenderContext *p_render_ctx) -> TextureManager *;
	auto TST_RENDER_API texture_mngr_destroy(TextureManager *p_mngr) -> void;
	auto TST_RENDER_API texture_mngr_poll_uploads(TextureManager *p_mngr, gpu::CommandListHandle p_cmd) -> void; // Generates texture mipmaps with the command buffer

	auto TST_RENDER_API texture_register(TextureManager *p_mngr, uint32 p_state_tracker_expected_subresources = textureSubresourceOneTimeUpload) -> TextureHandle;
	auto TST_RENDER_API texture_create(TextureManager *p_mngr, const gpu::TextureDesc &p_desc,
									   uint32          p_state_tracker_expected_subresources = textureSubresourceOneTimeUpload) -> TextureHandle;
	auto TST_RENDER_API texture_create_into(TextureManager *p_mngr, TextureHandle p_handle, const gpu::TextureDesc &p_desc) -> void;
	auto TST_RENDER_API texture_destroy(TextureManager *p_mngr, TextureHandle p_texture) -> void;

	auto TST_RENDER_API texture_set_data(TextureManager *p_mngr, TextureHandle p_texture, const void *p_data, uint64 p_size) -> void;
	auto TST_RENDER_API texture_wait_for_upload(TextureManager *p_mngr, TextureHandle p_texture) -> void;

	// You may want to use an already defined state tracker for a texture's upload, if so, remember to use textureSubresourceNoUpload to not create an initial state tracker
	auto TST_RENDER_API texture_set_state_tracker(TextureManager *p_mngr, TextureHandle p_texture, gpu::StateTrackerHandle p_state_tracker) -> void;

	auto TST_RENDER_API texture_get(TextureManager *p_mngr, TextureHandle p_texture) -> Texture *;
	auto TST_RENDER_API texture_try_get(TextureManager *p_mngr, TextureHandle p_texture) -> Texture *;

	auto TST_RENDER_API texture_get_mip_storage_heap_slot(TextureManager *p_mngr, TextureHandle p_texture, uint32 p_mip) -> uint32;
}
