#pragma once

#include <filesystem>

#include "texture_importer.hpp"
#include "toast_render/mesh.hpp"
#include "toast_asset.hpp"

namespace toaster::asset
{
	struct TST_ASSET_API MeshImportData
	{
		std::vector<render::StaticMeshVertex> vertices;
		std::vector<uint32>                   indices;
		std::vector<render::Submesh>          submeshes;
	};

	class TST_ASSET_API MeshImporter
	{
	public:
		MeshImporter(render::MeshManager* p_mesh_manager,render::MaterialManager* p_material_manager,TextureImporter *p_texture_importer );
		~MeshImporter();

		auto importStaticMeshDataFromFile(const std::filesystem::path &p_path) -> MeshImportData;

		auto asyncLoadStaticMeshFromFile(render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path) -> void;

		auto waitImports() -> void;

	private:
		NonOwningPtr<TextureImporter>         m_textureImporter{nullptr};
		NonOwningPtr<render::MeshManager>     m_meshManager{nullptr};
		NonOwningPtr<render::MaterialManager> m_materialManager{nullptr};
		NonOwningPtr<render::TextureManager>  m_textureManager{nullptr};

		struct ImportTask
		{
			String                   path;
			render::StaticMeshHandle dstMesh{nullptr};
		};

		std::atomic_bool m_terminationRequested{false};

		std::vector<std::thread> m_pendingImports;
	};
}
