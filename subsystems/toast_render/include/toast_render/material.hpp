#pragma once

#include <unordered_set>

#include "texture.hpp"
#include "toast_gpu/allocation.hpp"

#define falsity false // Is it a Verity?!

namespace toaster::rnd
{
	enum class EMaterialFlagBits : uint8
	{
		eBackfaceCulling = TST_LIB_BIT(0u),
		eTransparent     = TST_LIB_BIT(1u),
		eNoNormalMap     = TST_LIB_BIT(2u),
		eNoRoughnessMap  = TST_LIB_BIT(3u),
		eNoMetalnessMap  = TST_LIB_BIT(4u)
	};

	TST_LIB_SPECIALISE_FLAGS(EMaterialFlagBits, EMaterialFlags);

	struct TST_RENDER_API MaterialParams
	{
		math::Float4   albedoColour{1.0f, 1.0f, 1.0f, 1.0f};
		uint32         albedoMapHeapSlot{UINT32_MAX};
		uint32         normalMapHeapSlot{UINT32_MAX};
		uint32         roughnessMapHeapSlot{UINT32_MAX};
		uint32         metalnessMapHeapSlot{UINT32_MAX};
		float32        roughness{0.0f};
		float32        metalness{0.0f};
		EMaterialFlags flags{
			EMaterialFlagBits::eBackfaceCulling | EMaterialFlagBits::eNoNormalMap | EMaterialFlagBits::eNoRoughnessMap | EMaterialFlagBits::eNoMetalnessMap
		};
	};

	struct TST_RENDER_API Material
	{
		MaterialParams params; // Raw parameter data
		TextureHandle  albedoMap{nullptr};
		TextureHandle  normalMap{nullptr};
		TextureHandle  roughnessMap{nullptr};
		TextureHandle  metalnessMap{nullptr};
	};

	TST_DECLARE_HANDLE(Material);

	class TST_RENDER_API MaterialManager
	{
	public:
		MaterialManager(RenderContext *p_render_ctx, TextureManager *p_texture_mngr, uint32 p_max_materials, uint32 p_max_frames_in_flight = 3u);
		~MaterialManager();

		[[nodiscard]] auto createMaterial() -> MaterialHandle;
		auto               destroyMaterial(MaterialHandle p_handle) -> void;

		[[nodiscard]] auto getMaterial(MaterialHandle p_handle) -> Material & { return m_materials[p_handle]; }
		[[nodiscard]] auto getMaterial(MaterialHandle p_handle) const -> const Material & { return m_materials[p_handle]; }
		[[nodiscard]] auto tryGetMaterial(MaterialHandle p_handle) -> Material * { return m_materials.tryGet(p_handle); }
		[[nodiscard]] auto tryGetMaterial(MaterialHandle p_handle) const -> const Material * { return m_materials.tryGet(p_handle); }

		auto setAlbedoColour(MaterialHandle p_handle, const math::Float4 &p_colour) -> void;
		auto setAlbedoMap(MaterialHandle p_handle, TextureHandle p_albedo_map) -> void;
		auto setNormalMap(MaterialHandle p_handle, TextureHandle p_normal_map) -> void;
		auto setRoughnessMap(MaterialHandle p_handle, TextureHandle p_roughness_map) -> void;
		auto setMetalnessMap(MaterialHandle p_handle, TextureHandle p_metalness_map) -> void;
		auto setRoughness(MaterialHandle p_handle, float32 p_roughness) -> void;
		auto setMetalness(MaterialHandle p_handle, float32 p_metalness) -> void;

		auto setMaterialFlags(MaterialHandle p_handle, EMaterialFlags p_flags, bool p_enable) -> void;
		auto setBackfaceCulling(MaterialHandle p_handle, bool p_enable) -> void;
		auto setIsTransparent(MaterialHandle p_handle, bool p_is_transparent) -> void;
		auto setHasNormalMap(MaterialHandle p_handle, bool p_has_normal_map) -> void;
		auto setHasRoughnessMap(MaterialHandle p_handle, bool p_has_roughness_map) -> void;
		auto setHasMetalnessMap(MaterialHandle p_handle, bool p_has_metalness_map) -> void;

		auto pollMaterialTextureUploads() -> void;

		auto updateDirtyMaterials(uint32 p_frame_index) -> void;
		auto markMaterialDirty(MaterialHandle p_handle) -> void;

		[[nodiscard]] auto getMaterialBufferAddress(uint32 p_frame_index) const -> gpu::DeviceAddress { return m_bdas[p_frame_index]; }

		[[nodiscard]] auto getDefaultColourMap() const -> TextureHandle { return m_defaultColourMap; }
		[[nodiscard]] auto getDefaultNormalMap() const -> TextureHandle { return m_defaultNormalMap; }
		[[nodiscard]] auto getDefaultRoughnessMap() const -> TextureHandle { return m_defaultRoughnessMap; }
		[[nodiscard]] auto getDefaultMetalnessMap() const -> TextureHandle { return m_defaultMetalnessMap; }

	private:
		RenderContext * m_renderCtx{nullptr};
		TextureManager *m_textureMngr{nullptr};

		Pool<Material> m_materials;

		std::vector<gpu::DeviceAddress>                  m_bdas;
		std::vector<gpu::BufferHandle>                   m_materialSSBOs;
		std::vector<std::unordered_set<MaterialHandle> > m_dirtyMaterials; // Per frame

		enum class EPendingTextureType : uint8
		{
			eAlbedoMap,
			eNormalMap,
			eRoughnessMap,
			eMetalnessMap
		};

		std::unordered_map<MaterialHandle, std::unordered_set<EPendingTextureType> > m_pendingMaterialTextureUploads;
		// List of materials which have textures that are not fully uploaded or created

		uint32 m_maxFramesInFlight{3u};

		TextureHandle m_defaultColourMap{nullptr};
		TextureHandle m_defaultNormalMap{nullptr};
		TextureHandle m_defaultRoughnessMap{nullptr};
		TextureHandle m_defaultMetalnessMap{nullptr};

		std::mutex m_dirtyMaterialMutex;
		std::mutex m_textureUploadMutex;
	};
}
