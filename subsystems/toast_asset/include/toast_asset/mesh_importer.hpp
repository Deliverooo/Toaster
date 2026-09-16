#pragma once

#include <filesystem>

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
		MeshImporter() = default;
		~MeshImporter();

		static auto importStaticMeshDataFromFile(const std::filesystem::path &p_path) -> MeshImportData;

		auto asyncLoadStaticMeshFromFile(render::MeshManager *p_mesh_manager, render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path) -> void;

	private:
		struct ImportTask
		{
			String                   path;
			render::StaticMeshHandle dstMesh{nullptr};
		};

		std::atomic_bool m_terminationRequested{false};

		std::vector<std::thread> m_pendingImports;
	};
}
