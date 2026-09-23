#pragma once

#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "render_context.hpp"
#include "toast_gpu/upload.hpp"
#include "toast_lib/pool.hpp"

namespace toaster::render
{
	enum class ETextureState : uint8
	{
		eUnloaded,
		eLoading,
		eUploadingToGPU,
		eReady
	};

	struct TST_RENDER_API Texture
	{
		gpu::TextureHandle texture{nullptr};

		uint32 shaderReadHeapSlot{UINT32_MAX};
		uint32 storageHeapSlot{UINT32_MAX};

		std::unordered_map<uint32, uint32> perMipStorageHeapSlots;

		gpu::upload::StateTrackerHandle stateTracker{nullptr};
	};

	TST_DECLARE_HANDLE(Texture);

	class TST_RENDER_API TextureManager
	{
	public:
		TextureManager(RenderContext *p_render_ctx);
		~TextureManager();

		auto registerTexture() -> TextureHandle;
		auto createTexture(const gpu::TextureDesc &p_desc) -> TextureHandle;
		auto destroyTexture(TextureHandle p_handle) -> void;

		auto createIntoTexture(TextureHandle p_handle, const gpu::TextureDesc &p_desc) -> void;

		[[nodiscard]] auto getTexture(TextureHandle p_handle) -> Texture & { return m_textures[p_handle]; }
		[[nodiscard]] auto getTexture(TextureHandle p_handle) const -> const Texture & { return m_textures[p_handle]; }
		[[nodiscard]] auto tryGetTexture(TextureHandle p_handle) -> Texture * { return m_textures.tryGet(p_handle); }
		[[nodiscard]] auto tryGetTexture(TextureHandle p_handle) const -> const Texture * { return m_textures.tryGet(p_handle); }

		auto setData(TextureHandle p_handle, const void *p_data, uint64 p_size) -> void;

		// Gets the mip storage heap slot if present, else, it will create one.
		[[nodiscard]] auto getMipStorageHeapSlot(TextureHandle p_handle, uint32 p_mip) -> uint32;

		auto pollTextureUploads(gpu::CommandListHandle p_cmd) -> void; // Generates texture mipmaps with the command buffer

	private:
		NonOwningPtr<RenderContext> m_renderCtx{nullptr};

		Pool<Texture> m_textures;

		// std::vector<TextureHandle> m_pendingMipmapGenerations;
	};
}
