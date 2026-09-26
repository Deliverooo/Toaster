#pragma once

#include <unordered_set>

#include "texture.hpp"
#include "toast_gpu/allocation.hpp"

#define falsity false // Is it a Verity?!

namespace toaster::render
{
	// struct TST_RENDER_API MaterialParameter
	// {
	// 	String name;
	// 	uint32 offset{0u};
	// 	uint32 size{0u};
	// };
	//
	// using MaterialTemplateHandle = RefPtr<struct MaterialTemplate>;
	//
	// struct TST_RENDER_API MaterialTemplate
	// {
	// 	std::vector<MaterialParameter> parameters;
	// 	uint32                         totalSize{0u}; // Summed size of all the parameters
	//
	// 	static auto create(InitialiserList<const MaterialParameter> p_params) -> MaterialTemplateHandle
	// 	{
	// 		auto material_template{MaterialTemplateHandle{new MaterialTemplate{}}};
	//
	// 		material_template->parameters.resize(p_params.size());
	//
	// 		for (uint32 i{0u}; i < p_params.size(); ++i)
	// 		{
	// 			const auto &param{p_params[i]};
	//
	// 			material_template->parameters[i] = param;
	// 			material_template->totalSize     += param.size;
	// 		}
	//
	// 		return material_template;
	// 	}
	// };

	struct TST_RENDER_API alignas(16u) MaterialParams
	{
		tsm::float3 albedoColour{1.0f};
		uint32      _padd;
		uint32      albedoMapHeapSlot{UINT32_MAX};
		uint32      normalMapHeapSlot{UINT32_MAX};
		uint32      _padd2[2];
	};

	struct TST_RENDER_API Material
	{
		MaterialParams params; // Raw parameter data
		TextureHandle  albedoMap{nullptr};
		TextureHandle  normalMap{nullptr};
	};

	TST_DECLARE_HANDLE(Material);

	class TST_RENDER_API MaterialManager
	{
	public:
		MaterialManager(TextureManager *p_texture_manager, uint32 p_max_materials, uint32 p_max_frames_in_flight = 3u);
		~MaterialManager();

		[[nodiscard]] auto createMaterial() -> MaterialHandle;
		auto               destroyMaterial(MaterialHandle p_handle) -> void;

		[[nodiscard]] auto getMaterial(MaterialHandle p_handle) -> Material & { return m_materials[p_handle]; }
		[[nodiscard]] auto getMaterial(MaterialHandle p_handle) const -> const Material & { return m_materials[p_handle]; }
		[[nodiscard]] auto tryGetMaterial(MaterialHandle p_handle) -> Material * { return m_materials.tryGet(p_handle); }
		[[nodiscard]] auto tryGetMaterial(MaterialHandle p_handle) const -> const Material * { return m_materials.tryGet(p_handle); }

		auto setAlbedoColour(MaterialHandle p_handle, const tsm::float3 &p_colour) -> void;
		auto setAlbedoMap(MaterialHandle p_handle, TextureHandle p_albedo_map) -> void;
		auto setNormalMap(MaterialHandle p_handle, TextureHandle p_normal_map) -> void;

		auto pollMaterialTextureUploads() -> void;

		auto updateDirtyMaterials(uint32 p_frame_index) -> void;
		auto markMaterialDirty(MaterialHandle p_handle) -> void;

		[[nodiscard]] auto getMaterialBufferAddress(uint32 p_frame_index) const -> gpu::DeviceAddress { return m_bdas[p_frame_index]; }

		[[nodiscard]] auto getDefaultColourMap() const -> TextureHandle { return m_defaultColourMap; }
		[[nodiscard]] auto getDefaultNormalMap() const -> TextureHandle { return m_defaultNormalMap; }

	private:
		NonOwningPtr<TextureManager> m_textureManager{nullptr};

		Pool<Material> m_materials;

		std::vector<gpu::DeviceAddress>                  m_bdas;
		std::vector<gpu::BufferHandle>                   m_materialSSBOs;
		std::vector<std::unordered_set<MaterialHandle> > m_dirtyMaterials; // Per frame

		enum class EPendingTextureType
		{
			eAlbedoMap, eNormalMap
		};

		std::unordered_map<MaterialHandle, std::unordered_set<EPendingTextureType> > m_pendingMaterialTextureUploads;
		// List of materials which have textures that are not fully uploaded or created

		uint32 m_maxFramesInFlight{3u};

		TextureHandle m_defaultColourMap{nullptr};
		TextureHandle m_defaultNormalMap{nullptr};

		std::mutex m_dirtyMaterialMutex;
		std::mutex m_textureUploadMutex;
	};
}
