#pragma once

#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "render_context.hpp"
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

		ETextureState state{ETextureState::eUnloaded};
		uint64        transferReadyToken{0u};
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

		// Thread safe operations
		auto getTextureState(TextureHandle p_handle) -> ETextureState;
		auto setTextureState(TextureHandle p_handle, ETextureState p_state) -> void;

		struct TextureThreadData
		{
			gpu::TextureHandle texture{nullptr};

			uint32 shaderReadHeapSlot{UINT32_MAX};
			uint32 storageHeapSlot{UINT32_MAX};

			std::unordered_map<uint32, uint32> perMipStorageHeapSlots;

			ETextureState state{ETextureState::eUnloaded};
			uint64        transferReadyToken{0u};
		};

		// Returns a copy of the static texture's data, so the operation can be done under the internal mutex's lock
		auto getTextureThreadData(TextureHandle p_handle) -> TextureThreadData;
		auto tryGetTextureThreadData(TextureHandle p_handle) -> std::optional<TextureThreadData>;

		auto pollTextureUploads() -> void;

	private:
		NonOwningPtr<RenderContext> m_renderCtx{nullptr};

		std::unordered_set<TextureHandle> m_pendingTextureUploads;

		Pool<Texture> m_textures;

		std::mutex m_mutex;
	};
}
