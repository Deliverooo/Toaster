#include "toast_render/material.hpp"

#include <algorithm>

#include "toast_gpu/frame.hpp"

namespace toaster::render
{
	MaterialManager::MaterialManager(uint64 p_max_material_block_size, uint32 p_max_frames_in_flight) : m_maxMaterialBlockSize(p_max_material_block_size),
																										m_maxFramesInFlight(p_max_frames_in_flight)
	{
		m_virtualBlock = gpu::alloc::createVirtualBlock(m_maxMaterialBlockSize);

		gpu::BufferDesc material_buffer_desc{};
		material_buffer_desc.size       = m_maxMaterialBlockSize;
		material_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		material_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		m_materialSSBOs.resize(m_maxFramesInFlight);
		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_materialSSBOs[i] = gpu::createBuffer(material_buffer_desc);

		m_materials.setDestructorFn(+[](Material *p_data, void *) -> void
		{
			gpu::alloc::virtualFree(p_data->allocation); // Ts is virtual, so it doesn't need to be deferred

			p_data->materialTemplate.reset();
			p_data->data.clear();
		});
	}

	MaterialManager::~MaterialManager()
	{
		for (auto ssbo: m_materialSSBOs)
			gpu::frame::defferBufferDeletion(ssbo);

		gpu::alloc::destroyVirtualBlock(m_virtualBlock);

		m_materials.clear();
	}

	auto MaterialManager::createMaterial(const MaterialTemplateHandle &p_template) -> MaterialHandle
	{
		Material material{};
		material.data.resize(p_template->totalSize);
		material.materialTemplate = p_template;
		material.allocation       = gpu::alloc::virtualAllocate(m_virtualBlock, p_template->totalSize, 16u); // std430 alignment

		MaterialHandle out_handle{m_materials.emplace(std::move(material))}; // Move because of the vector
		markMaterialDirty(out_handle);                                       // The material should initially be dirty, even if no parameters were changed
		return out_handle;
	}

	auto MaterialManager::destroyMaterial(MaterialHandle p_handle) -> void
	{
		m_materials.destroy(p_handle);
	}

	auto MaterialManager::setParameter(MaterialHandle p_handle, StringView p_name, const void *p_data) -> void
	{
		Material &material{m_materials[p_handle]};

		const auto &params{material.materialTemplate->parameters};

		const auto it{
			std::ranges::find_if(params, [p_name](const auto &p_param) -> bool
			{
				return p_param.name == p_name;
			})
		};

		if (it == params.end())
		{
			TST_ASSERT_MSG(false, "Failed to find material parameter");
			return;
		}
		std::memcpy(material.data.data() + it->offset, p_data, it->size);
		markMaterialDirty(p_handle);
	}

	auto MaterialManager::updateDirtyMaterials(uint32 p_frame_index) -> void
	{
		auto &dirty_list{m_dirtyMaterials[p_frame_index]};
		if (dirty_list.empty())
			return;

		for (auto handle: dirty_list)
		{
			const Material *material{m_materials.tryGet(handle)}; // The material may have been destroyed prior to calling this function
			if (!material)
				continue;

			uint64 allocation_size{gpu::alloc::getAllocationSize(material->allocation)};
			uint64 allocation_offset{gpu::alloc::getAllocationOffset(material->allocation)};
			gpu::writeBufferData(m_materialSSBOs[p_frame_index], material->data.data(), allocation_size, allocation_offset);
		}
		dirty_list.clear();
	}

	auto MaterialManager::markMaterialDirty(MaterialHandle p_handle) -> void
	{
		for (uint32 i{0u}; i < m_maxFramesInFlight; ++i)
			m_dirtyMaterials[i].insert(p_handle); // Inserts if it is not already present
	}

	auto MaterialManager::getMaterialBufferAddress(MaterialHandle p_handle, uint32 p_frame_index) const -> gpu::DeviceAddress
	{
		const Material &         material{m_materials[p_handle]};
		const gpu::DeviceAddress base_address{gpu::getBufferAddress(m_materialSSBOs[p_frame_index])};
		const uint64             allocation_offset{gpu::alloc::getAllocationOffset(material.allocation)};
		return base_address + allocation_offset;
	}
}
