#pragma once

#include <filesystem>
#include <taskflow/taskflow.hpp>

#include "texture_importer.hpp"
#include "toast_render/mesh.hpp"
#include "toast_asset.hpp"
#include "toast_lib/filesystem.hpp"

namespace toaster::asset
{
	struct TST_ASSET_API TextureImportData
	{
		std::optional<filesystem::Path> path;
		// TODO: Embedded textures with std::vector<uint8> data
	};

	struct TST_ASSET_API MaterialImportData // CPU-side material data to pass off to the GPU
	{
		TextureImportData albedoMap;
		TextureImportData normalMap;

		tsm::float4 albedoColour;
	};

	struct TST_ASSET_API MeshImportData
	{
		std::vector<render::StaticMeshVertex> vertices;
		std::vector<render::Meshlet>          meshlets;
		std::vector<uint32>                   meshletVertices;
		std::vector<uint8>                    meshletTriangles;
		std::vector<MaterialImportData>       materials;

		XMFLOAT4 boundingSphere;
	};

	class TST_ASSET_API MeshImporter
	{
	public:
		MeshImporter(render::MeshManager *p_mesh_manager, render::MaterialManager *p_material_manager, render::TextureManager *p_texture_manager);

		static auto importStaticMeshDataFromFile(const std::filesystem::path &p_path, RefPtr<MeshImportData> &p_out_data) -> void;

		// The callback will probably be used to set material properties just after loading. This does not mean that the GPU uploads have completed, only the mesh parsing has
		auto asyncLoadStaticMeshFromFile(render::StaticMeshHandle                             p_dst_mesh, const std::filesystem::path &p_path, tf::Executor &p_executor,
										 const std::function<void(render::StaticMeshHandle)> &p_on_finish_cb = nullptr) -> void;

	private:
		NonOwningPtr<render::MeshManager>     m_meshManager{nullptr};
		NonOwningPtr<render::MaterialManager> m_materialManager{nullptr};
		NonOwningPtr<render::TextureManager>  m_textureManager{nullptr};
	};
}
