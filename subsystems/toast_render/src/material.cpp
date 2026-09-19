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

		m_materials.setDestructorFn(+[](Material *p_data, void *) -> void
		{
		});

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

			uint32 normal_data{0xFFFFFFFF};
			m_textureManager->setData(m_defaultNormalMap, &normal_data, sizeof(uint32));
		}

		gpu::upload::flushUploadsAndWait();
		m_textureManager->pollTextureUploads();
	}

	MaterialManager::~MaterialManager()
	{
		for (auto ssbo: m_materialSSBOs)
			gpu::frame::defferBufferDeletion(ssbo);

		m_materials.clear();
	}

	auto MaterialManager::createMaterial() -> MaterialHandle
	{
		std::scoped_lock<std::mutex> lock{m_mutex};

		Material material{};
		material.albedoMap                = m_defaultColourMap;
		material.normalMap                = m_defaultNormalMap;
		material.params.albedoMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(m_defaultColourMap);
		material.params.normalMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(m_defaultNormalMap);

		MaterialHandle out_handle{m_materials.emplace(material)};

		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(out_handle); // Inserts if it is not already present
		return out_handle;
	}

	auto MaterialManager::destroyMaterial(MaterialHandle p_handle) -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};

		m_materials.destroy(p_handle);
	}

	auto MaterialManager::setAlbedoColour(MaterialHandle p_handle, const tsm::float3 &p_colour) -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};
		Material &                   material{m_materials[p_handle]};

		material.params.albedoColour = p_colour;

		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(p_handle); // Inserts if it is not already present
	}

	auto MaterialManager::setAlbedoMap(MaterialHandle p_handle, TextureHandle p_albedo_map) -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};
		Material &                   material{m_materials[p_handle]};

		material.albedoMap = p_albedo_map;

		if (m_textureManager->getTextureState(p_albedo_map) != ETextureState::eReady)
		{
			m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eAlbedoMap);
			material.params.albedoMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(m_defaultColourMap);
		}
		else
			material.params.albedoMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(p_albedo_map);

		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(p_handle); // Inserts if it is not already present
	}

	auto MaterialManager::setNormalMap(MaterialHandle p_handle, TextureHandle p_normal_map) -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};
		Material &                   material{m_materials[p_handle]};

		material.normalMap = p_normal_map;

		if (m_textureManager->getTextureState(p_normal_map) != ETextureState::eReady)
		{
			m_pendingMaterialTextureUploads[p_handle].insert(EPendingTextureType::eNormalMap);
			material.params.normalMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(m_defaultNormalMap);
		}
		else
			material.params.normalMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(p_normal_map);

		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(p_handle); // Inserts if it is not already present
	}

	auto MaterialManager::pollMaterialTextureUploads() -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};

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
					if (m_textureManager->getTextureState(mat.albedoMap) == ETextureState::eReady)
					{
						mat.params.albedoMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(mat.albedoMap);
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				else if (*type_it == EPendingTextureType::eNormalMap)
				{
					if (m_textureManager->getTextureState(mat.normalMap) == ETextureState::eReady)
					{
						mat.params.normalMapHeapSlot = m_textureManager->getTextureShaderReadHeapSlot(mat.normalMap);
						need_update                  = true;
						type_it                      = types.erase(type_it);
						continue;
					}
				}
				++type_it;
			}

			if (need_update)
			{
				for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
					m_dirtyMaterials[i].insert(handle); // Inserts if it is not already present
			}
		}

		std::erase_if(m_pendingMaterialTextureUploads, [](const auto &pair) -> bool
		{
			return pair.second.empty();
		});
	}

	auto MaterialManager::updateDirtyMaterials(uint32 p_frame_index) -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};

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
		std::scoped_lock<std::mutex> lock{m_mutex};

		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(p_handle); // Inserts if it is not already present
	}
}
