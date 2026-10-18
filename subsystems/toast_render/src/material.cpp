#include "toast_render/material.hpp"

#include <algorithm>

#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	MaterialManager::MaterialManager(TextureManager *p_texture_manager, uint32 p_max_materials, uint32 p_max_frames_in_flight) : m_textureManager(p_texture_manager),
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
			m_materialSSBOs[i] = gpu::createBuffer(material_buffer_desc);
			m_bdas[i]          = gpu::getBufferAddress(m_materialSSBOs[i]);
		}

		m_dirtyMaterials.resize(m_maxFramesInFlight);

		{
			gpu::TextureDesc colour_texture_desc{};
			colour_texture_desc.extent      = {1u, 1u, 1u};
			colour_texture_desc.mipCount    = 1u;
			colour_texture_desc.layerCount  = 1u;
			colour_texture_desc.type        = gpu::ETextureType::e2D;
			colour_texture_desc.sampleCount = gpu::ESampleCount::e1;
			colour_texture_desc.format      = gpu::EFormat::eR8G8B8A8Srgb;
			colour_texture_desc.usage       = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			m_defaultColourMap              = m_textureManager->createTexture(colour_texture_desc);

			uint32 colour_data{0xFFFFFFFF};
			m_textureManager->setData(m_defaultColourMap, &colour_data, sizeof(uint32));
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
			m_defaultNormalMap              = m_textureManager->createTexture(normal_texture_desc);

			uint32 normal_data{0xFFFF0000};
			m_textureManager->setData(m_defaultNormalMap, &normal_data, sizeof(uint32));
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
			m_defaultRoughnessMap              = m_textureManager->createTexture(roughness_texture_desc);

			uint8 roughness_data{0xFF};
			m_textureManager->setData(m_defaultRoughnessMap, &roughness_data, sizeof(uint8));
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
			m_defaultMetalnessMap              = m_textureManager->createTexture(metalness_texture_desc);

			uint8 metalness_data{0x00};
			m_textureManager->setData(m_defaultMetalnessMap, &metalness_data, sizeof(uint8));
		}
	}

	MaterialManager::~MaterialManager()
	{
		for (auto ssbo: m_materialSSBOs)
			gpu::frame::deferBufferDeletion(ssbo);

		m_materials.clear();
	}

	auto MaterialManager::createMaterial() -> MaterialHandle
	{
		Material material{};
		material.albedoMap                   = m_defaultColourMap;
		material.normalMap                   = m_defaultNormalMap;
		material.params.albedoMapHeapSlot    = m_textureManager->getTexture(m_defaultColourMap).shaderReadHeapSlot;
		material.params.normalMapHeapSlot    = m_textureManager->getTexture(m_defaultNormalMap).shaderReadHeapSlot;
		material.params.roughnessMapHeapSlot = m_textureManager->getTexture(m_defaultRoughnessMap).shaderReadHeapSlot;
		material.params.metalnessMapHeapSlot = m_textureManager->getTexture(m_defaultMetalnessMap).shaderReadHeapSlot;
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

	auto MaterialManager::setAlbedoColour(MaterialHandle p_handle, const tsm::float4 &p_colour) -> void
	{
		Material &material{m_materials[p_handle]};
		material.params.albedoColour = p_colour;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setAlbedoMap(MaterialHandle p_handle, TextureHandle p_albedo_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.albedoMap = p_albedo_map;

		if (!gpu::upload::isStateTrackerReady(m_textureManager->getTexture(p_albedo_map).stateTracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eAlbedoMap);
			}

			material.params.albedoMapHeapSlot = m_textureManager->getTexture(m_defaultColourMap).shaderReadHeapSlot;
		}
		else
			material.params.albedoMapHeapSlot = m_textureManager->getTexture(p_albedo_map).shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setNormalMap(MaterialHandle p_handle, TextureHandle p_normal_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.params.flags &= ~EMaterialFlagBits::eNoNormalMap;

		material.normalMap = p_normal_map;

		if (!gpu::upload::isStateTrackerReady(m_textureManager->getTexture(p_normal_map).stateTracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eNormalMap);
			}

			material.params.normalMapHeapSlot = m_textureManager->getTexture(m_defaultNormalMap).shaderReadHeapSlot;
		}
		else
			material.params.normalMapHeapSlot = m_textureManager->getTexture(p_normal_map).shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setRoughnessMap(MaterialHandle p_handle, TextureHandle p_roughness_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.params.flags &= ~EMaterialFlagBits::eNoRoughnessMap;

		material.roughnessMap = p_roughness_map;

		if (!gpu::upload::isStateTrackerReady(m_textureManager->getTexture(p_roughness_map).stateTracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eRoughnessMap);
			}

			material.params.roughnessMapHeapSlot = m_textureManager->getTexture(m_defaultRoughnessMap).shaderReadHeapSlot;
		}
		else
			material.params.roughnessMapHeapSlot = m_textureManager->getTexture(p_roughness_map).shaderReadHeapSlot;

		markMaterialDirty(p_handle);
	}

	auto MaterialManager::setMetalnessMap(MaterialHandle p_handle, TextureHandle p_metalness_map) -> void
	{
		Material &material{m_materials[p_handle]};

		material.params.flags &= ~EMaterialFlagBits::eNoMetalnessMap;

		material.metalnessMap = p_metalness_map;

		if (!gpu::upload::isStateTrackerReady(m_textureManager->getTexture(p_metalness_map).stateTracker))
		{
			{
				std::scoped_lock<std::mutex> lock{m_textureUploadMutex};
				m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eMetalnessMap);
			}

			material.params.metalnessMapHeapSlot = m_textureManager->getTexture(m_defaultMetalnessMap).shaderReadHeapSlot;
		}
		else
			material.params.metalnessMapHeapSlot = m_textureManager->getTexture(p_metalness_map).shaderReadHeapSlot;

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
					if (gpu::upload::isStateTrackerReady(m_textureManager->getTexture(mat.albedoMap).stateTracker))
					{
						mat.params.albedoMapHeapSlot = m_textureManager->getTexture(mat.albedoMap).shaderReadHeapSlot;
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eNormalMap)
				{
					if (gpu::upload::isStateTrackerReady(m_textureManager->getTexture(mat.normalMap).stateTracker))
					{
						mat.params.normalMapHeapSlot = m_textureManager->getTexture(mat.normalMap).shaderReadHeapSlot;
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eRoughnessMap)
				{
					if (gpu::upload::isStateTrackerReady(m_textureManager->getTexture(mat.roughnessMap).stateTracker))
					{
						mat.params.roughnessMapHeapSlot = m_textureManager->getTexture(mat.roughnessMap).shaderReadHeapSlot;
						need_update                     = true;
						type_it                         = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eMetalnessMap)
				{
					if (gpu::upload::isStateTrackerReady(m_textureManager->getTexture(mat.metalnessMap).stateTracker))
					{
						mat.params.metalnessMapHeapSlot = m_textureManager->getTexture(mat.metalnessMap).shaderReadHeapSlot;
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

			gpu::writeBufferData(m_materialSSBOs[p_frame_index], &material->params, sizeof(MaterialParams), sizeof(MaterialParams) * handle.getId());
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
