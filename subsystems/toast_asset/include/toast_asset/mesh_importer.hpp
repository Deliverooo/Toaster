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
		static auto importStaticFromFile(const std::filesystem::path &p_path) -> MeshImportData;
	};
}
