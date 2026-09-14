#pragma once

#include <unordered_set>

#include "texture.hpp"
#include "toast_gpu/allocation.hpp"

#define falsity false // Is it a Verity?!

namespace toaster::render
{
	struct TST_RENDER_API MaterialParameter
	{
		String name;
		uint32 offset{0u};
		uint32 size{0u};
	};

	// The material template does not need atomic reference counting. Uint32 is just fine
	using MaterialTemplateHandle = RefPtr<struct MaterialTemplate, uint32>;

	struct TST_RENDER_API MaterialTemplate
	{
		std::vector<MaterialParameter> parameters;
		uint32                         totalSize{0u}; // Summed size of all the parameters

		static auto create(InitialiserList<const MaterialParameter> p_params) -> MaterialTemplateHandle
		{
			auto material_template{MaterialTemplateHandle{new MaterialTemplate{}}};

			material_template->parameters.resize(p_params.size());

			for (uint32 i{0u}; i < p_params.size(); ++i)
			{
				const auto &param{p_params[i]};

				material_template->parameters[i] = param;
				material_template->totalSize     += param.size;
			}

			return material_template;
		}
	};

	struct TST_RENDER_API Material
	{
		std::vector<uint8> data; // Raw parameter data

		MaterialTemplateHandle materialTemplate{nullptr}; // Maybe I shouldn't be using a RefPtr

		gpu::alloc::VirtualAllocationHandle allocation{nullptr};
	};

	TST_DECLARE_HANDLE(Material);

	class TST_RENDER_API MaterialManager
	{
	public:
		MaterialManager(uint64 p_max_material_block_size = 10u * 1028u * 1028u /*10MB*/, uint32 p_max_frames_in_flight = 3u);
		~MaterialManager();

		[[nodiscard]] auto createMaterial(const MaterialTemplateHandle &p_template) -> MaterialHandle;
		auto               destroyMaterial(MaterialHandle p_handle) -> void;

		[[nodiscard]] auto getMaterial(MaterialHandle p_handle) -> Material & { return m_materials[p_handle]; }
		[[nodiscard]] auto getMaterial(MaterialHandle p_handle) const -> const Material & { return m_materials[p_handle]; }
		[[nodiscard]] auto tryGetMaterial(MaterialHandle p_handle) -> Material * { return m_materials.tryGet(p_handle); }
		[[nodiscard]] auto tryGetMaterial(MaterialHandle p_handle) const -> const Material * { return m_materials.tryGet(p_handle); }

		auto setParameter(MaterialHandle p_handle, StringView p_name, const void *p_data) -> void;

		template<typename Type>
		auto setParameter(MaterialHandle p_handle, StringView p_name, const Type &p_data) -> void
		{
			setParameter(p_handle, p_name, &p_data);
		}

		auto updateDirtyMaterials(uint32 p_frame_index) -> void;
		auto markMaterialDirty(MaterialHandle p_handle) -> void;

		auto getMaterialBufferAddress(MaterialHandle p_handle, uint32 p_frame_index) const -> gpu::DeviceAddress;

	private:
		Pool<Material> m_materials;

		uint64                         m_maxMaterialBlockSize{0u};
		gpu::alloc::VirtualBlockHandle m_virtualBlock{nullptr};

		std::vector<gpu::BufferHandle>                   m_materialSSBOs{nullptr};
		std::vector<std::unordered_set<MaterialHandle> > m_dirtyMaterials; // Per frame

		uint32 m_maxFramesInFlight{3u};
	};
}
