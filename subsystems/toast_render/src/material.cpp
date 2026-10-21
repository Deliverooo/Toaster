#include "toast_render/material.hpp"

#include <algorithm>

#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::rnd
{
	MaterialManager::MaterialManager(RenderContext *p_render_ctx, TextureManager *p_texture_mngr, uint32 p_max_materials,
									 uint32         p_max_frames_in_flight) : m_renderCtx(p_render_ctx), m_textureMngr(p_texture_mngr),
																			  m_maxFramesInFlight(p_max_frames_in_flight)
	{
		gpu::BufferDesc material_buffer_desc{};
		material_buffer_desc.size       = sizeof(MaterialParams) * p_max_materials;
		material_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		material_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		m_materialSSBOs.resize(m_maxFramesInFlight);
		m_bdas.resize(m_maxFramesInFlight);
		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
		{
			m_materialSSBOs[i] = gpu::buffer_create(m_renderCtx->gpuCtx, material_buffer_desc);
			m_bdas[i]          = gpu::buffer_get_address(m_renderCtx->gpuCtx, m_materialSSBOs[i]);
		}

		m_dirtyMaterials.resize(m_maxFramesInFlight);

		// Helps by batching default uploads into one state to be tracked and waited on
		gpu::StateTrackerHandle default_resource_state_tracker{gpu::state_tracker_register(p_render_ctx->uploadCtx, 4u)};

		{
			gpu::TextureDesc colour_texture_desc{};
			colour_texture_desc.extent      = {1u, 1u, 1u};
			colour_texture_desc.mipCount    = 1u;
			colour_texture_desc.layerCount  = 1u;
			colour_texture_desc.type        = gpu::ETextureType::e2D;
			colour_texture_desc.sampleCount = gpu::ESampleCount::e1;
			colour_texture_desc.format      = gpu::EFormat::eR8G8B8A8Srgb;
			colour_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			m_defaultColourMap              = texture_create(m_textureMngr, colour_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(m_textureMngr, m_defaultColourMap, default_resource_state_tracker);

			uint32 colour_data{0xFFFFFFFF};
			texture_set_data(m_textureMngr, m_defaultColourMap, &colour_data, sizeof(uint32));
		}
		{
			gpu::TextureDesc normal_texture_desc{};
			normal_texture_desc.extent      = {1u, 1u, 1u};
			normal_texture_desc.mipCount    = 1u;
			normal_texture_desc.layerCount  = 1u;
			normal_texture_desc.type        = gpu::ETextureType::e2D;
			normal_texture_desc.sampleCount = gpu::ESampleCount::e1;
			normal_texture_desc.format      = gpu::EFormat::eR8G8B8A8Unorm;
			normal_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			m_defaultNormalMap              = texture_create(m_textureMngr, normal_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(m_textureMngr, m_defaultNormalMap, default_resource_state_tracker);

			uint32 normal_data{0xFFFF0000};
			texture_set_data(m_textureMngr, m_defaultNormalMap, &normal_data, sizeof(uint32));
		}
		{
			gpu::TextureDesc roughness_texture_desc{};
			roughness_texture_desc.extent      = {1u, 1u, 1u};
			roughness_texture_desc.mipCount    = 1u;
			roughness_texture_desc.layerCount  = 1u;
			roughness_texture_desc.type        = gpu::ETextureType::e2D;
			roughness_texture_desc.sampleCount = gpu::ESampleCount::e1;
			roughness_texture_desc.format      = gpu::EFormat::eR8Unorm;
			roughness_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			m_defaultRoughnessMap              = texture_create(m_textureMngr, roughness_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(m_textureMngr, m_defaultRoughnessMap, default_resource_state_tracker);

			uint8 roughness_data{0xFF};
			texture_set_data(m_textureMngr, m_defaultRoughnessMap, &roughness_data, sizeof(uint8));
		}
		{
			gpu::TextureDesc metalness_texture_desc{};
			metalness_texture_desc.extent      = {1u, 1u, 1u};
			metalness_texture_desc.mipCount    = 1u;
			metalness_texture_desc.layerCount  = 1u;
			metalness_texture_desc.type        = gpu::ETextureType::e2D;
			metalness_texture_desc.sampleCount = gpu::ESampleCount::e1;
			metalness_texture_desc.format      = gpu::EFormat::eR8Unorm;
			metalness_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			m_defaultMetalnessMap              = texture_create(m_textureMngr, metalness_texture_desc, textureSubresourceNoUpload);

			// Use the batch state tracker
			texture_set_state_tracker(m_textureMngr, m_defaultMetalnessMap, default_resource_state_tracker);

			uint8 metalness_data{0x00};
			texture_set_data(m_textureMngr, m_defaultMetalnessMap, &metalness_data, sizeof(uint8));
		}

		gpu::state_tracker_wait_for(p_render_ctx->uploadCtx, default_resource_state_tracker);
	}

	MaterialManager::~MaterialManager()
	{
		for (auto ssbo: m_materialSSBOs)
			gpu::defer_buffer_deletion(m_renderCtx->frameCtx, ssbo);

		m_materials.clear();
	}

	auto MaterialManager::createMaterial() -> MaterialHandle
	{
		Material material{};
		material.albedoMap                   = m_defaultColourMap;
		material.normalMap                   = m_defaultNormalMap;
		material.params.albedoMapHeapSlot    = texture_get(m_textureMngr, m_defaultColourMap)->shaderReadHeapSlot;
		material.params.normalMapHeapSlot    = texture_get(m_textureMngr, m_defaultNormalMap)->shaderReadHeapSlot;
		material.params.roughnessMapHeapSlot = texture_get(m_textureMngr, m_defaultRoughnessMap)->shaderReadHeapSlot;
		material.params.metalnessMapHeapSlot = texture_get(m_textureMngr, m_defaultMetalnessMap)->shaderReadHeapSlot;
		material.params.roughness            = 0.0f;
		material.params.metalness            = 0.0f;
		material.params.flags                = EMaterialFlagBits::eBackfaceCulling | EMaterialFlagBits::eNoNormalMap | EMaterialFlagBits::eNoRoughnessMap |
											   EMaterialFlagBits::eNoMetalnessMap;

		MaterialHandle out_handle{m_materials.emplace(material)};
		markMaterialDirty(out_handle);

		return out_handle;
	}

	auto MaterialManager::destroyMaterial(MaterialHandle p_handle) -> void
	{
		m_materials.destroy(p_handle);
	}

	auto MaterialManager::setAlbedoColour(MaterialHandle p_handle, const math::Float4 &p_colour) -> void
	{
		Material &material{m_materials[p_handle]};
		material.params.albedoColour = p_colour;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setAlbedoMap(MaterialHandle p_handle, TextureHandle p_albedo_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.albedoMap = p_albedo_map;

		const Texture *albedo_map{texture_get(m_textureMngr, p_albedo_map)};

		const gpu::StateTrackerHandle state_tracker{albedo_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eAlbedoMap);
			}

			material.params.albedoMapHeapSlot = texture_get(m_textureMngr, m_defaultColourMap)->shaderReadHeapSlot;
		}
		else
			material.params.albedoMapHeapSlot = albedo_map->shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setNormalMap(MaterialHandle p_handle, TextureHandle p_normal_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.params.flags &= ~EMaterialFlagBits::eNoNormalMap;
		material.normalMap    = p_normal_map;

		const Texture *normal_map{texture_get(m_textureMngr, p_normal_map)};

		const gpu::StateTrackerHandle state_tracker{normal_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eNormalMap);
			}

			material.params.normalMapHeapSlot = texture_get(m_textureMngr, m_defaultNormalMap)->shaderReadHeapSlot;
		}
		else
			material.params.normalMapHeapSlot = normal_map->shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setRoughnessMap(MaterialHandle p_handle, TextureHandle p_roughness_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.params.flags &= ~EMaterialFlagBits::eNoRoughnessMap;
		material.roughnessMap = p_roughness_map;

		const Texture *roughness_map{texture_get(m_textureMngr, p_roughness_map)};

		const gpu::StateTrackerHandle state_tracker{roughness_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eRoughnessMap);
			}

			material.params.roughnessMapHeapSlot = texture_get(m_textureMngr, m_defaultRoughnessMap)->shaderReadHeapSlot;
		}
		else
			material.params.roughnessMapHeapSlot = roughness_map->shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setMetalnessMap(MaterialHandle p_handle, TextureHandle p_metalness_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.params.flags &= ~EMaterialFlagBits::eNoMetalnessMap;
		material.metalnessMap = p_metalness_map;

		const Texture *metalness_map{texture_get(m_textureMngr, p_metalness_map)};

		const gpu::StateTrackerHandle state_tracker{metalness_map->stateTracker};
		// If there is no state tracker, the texture should be considered "ready"
		if (state_tracker && !gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eMetalnessMap);
			}

			material.params.metalnessMapHeapSlot = texture_get(m_textureMngr, m_defaultMetalnessMap)->shaderReadHeapSlot;
		}
		else
			material.params.metalnessMapHeapSlot = metalness_map->shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setRoughness(MaterialHandle p_handle, float32 p_roughness) -> void
	{
		Material &material{m_materials[p_handle]};
		material.params.roughness = p_roughness;
		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setMetalness(MaterialHandle p_handle, float32 p_metalness) -> void
	{
		Material &material{m_materials[p_handle]};
		material.params.metalness = p_metalness;
		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setMaterialFlags(MaterialHandle p_handle, EMaterialFlags p_flags, bool p_enable) -> void
	{
		Material &material{m_materials[p_handle]};
		if (p_enable)
			material.params.flags |= p_flags;
		else
			material.params.flags &= ~p_flags;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setBackfaceCulling(MaterialHandle p_handle, bool p_enable) -> void
	{
		setMaterialFlags(p_handle, EMaterialFlagBits::eBackfaceCulling, p_enable);
	}

	auto MaterialManager::setIsTransparent(MaterialHandle p_handle, bool p_is_transparent) -> void
	{
		setMaterialFlags(p_handle, EMaterialFlagBits::eTransparent, p_is_transparent);
	}

	auto MaterialManager::setHasNormalMap(MaterialHandle p_handle, bool p_has_normal_map) -> void
	{
		setMaterialFlags(p_handle, EMaterialFlagBits::eNoNormalMap, !p_has_normal_map);
	}

	auto MaterialManager::setHasRoughnessMap(MaterialHandle p_handle, bool p_has_roughness_map) -> void
	{
		setMaterialFlags(p_handle, EMaterialFlagBits::eNoRoughnessMap, !p_has_roughness_map);
	}

	auto MaterialManager::setHasMetalnessMap(MaterialHandle p_handle, bool p_has_metalness_map) -> void
	{
		setMaterialFlags(p_handle, EMaterialFlagBits::eNoMetalnessMap, !p_has_metalness_map);
	}

	auto MaterialManager::pollMaterialTextureUploads() -> void
	{
		std::scoped_lock<std::mutex> lock{m_textureUploadMutex};

		if (m_pendingMaterialTextureUploads.empty())
			return;

		for (auto &[handle, types]: m_pendingMaterialTextureUploads)
		{
			Material &mat{m_materials[handle]};

			bool need_update{false};
			for (auto type_it{types.begin()}; type_it != types.end();)
			{
				if (*type_it == EPendingTextureType::eAlbedoMap)
				{
					const Texture *               albedo_map{texture_get(m_textureMngr, mat.albedoMap)};
					const gpu::StateTrackerHandle state_tracker{albedo_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
					{
						mat.params.albedoMapHeapSlot = albedo_map->shaderReadHeapSlot;
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eNormalMap)
				{
					const Texture *               normal_map{texture_get(m_textureMngr, mat.normalMap)};
					const gpu::StateTrackerHandle state_tracker{normal_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
					{
						mat.params.normalMapHeapSlot = normal_map->shaderReadHeapSlot;
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eRoughnessMap)
				{
					const Texture *               roughness_map{texture_get(m_textureMngr, mat.roughnessMap)};
					const gpu::StateTrackerHandle state_tracker{roughness_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
					{
						mat.params.roughnessMapHeapSlot = roughness_map->shaderReadHeapSlot;
						need_update                     = true;
						type_it                         = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eMetalnessMap)
				{
					const Texture *               metalness_map{texture_get(m_textureMngr, mat.metalnessMap)};
					const gpu::StateTrackerHandle state_tracker{metalness_map->stateTracker};
					// If the state tracker was removed, the texture should be considered "ready"
					if (!state_tracker || gpu::state_tracker_is_ready(m_renderCtx->uploadCtx, state_tracker))
					{
						mat.params.metalnessMapHeapSlot = metalness_map->shaderReadHeapSlot;
						need_update                     = true;
						type_it                         = types.erase(type_it);
						continue;
					}
				}
				++type_it;
			}

			if (need_update)
				markMaterialDirty(handle);
		}

		// Erase all the uploads that are ready
		std::erase_if(m_pendingMaterialTextureUploads, [](const auto &pair) -> bool { return pair.second.empty(); });
	}

	auto MaterialManager::updateDirtyMaterials(uint32 p_frame_index) -> void
	{
		std::scoped_lock<std::mutex> lock{m_dirtyMaterialMutex};

		auto &dirty_list{m_dirtyMaterials[p_frame_index]};
		if (dirty_list.empty())
			return;

		for (auto handle: dirty_list)
		{
			const Material *material{m_materials.tryGet(handle)}; // The material may have been destroyed prior to calling this function
			if (!material)
				continue;

			gpu::buffer_write_data(m_renderCtx->gpuCtx, m_materialSSBOs[p_frame_index], &material->params, sizeof(MaterialParams),
								   sizeof(MaterialParams) * handle.getId());
		}
		dirty_list.clear();
	}

	auto MaterialManager::markMaterialDirty(MaterialHandle p_handle) -> void
	{
		std::scoped_lock<std::mutex> lock{m_dirtyMaterialMutex};

		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(p_handle); // Inserts if it is not already present
	}
}
