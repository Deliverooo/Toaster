#pragma once

#include <filesystem>

#include "toast_render/mesh.hpp"
#include "toast_asset.hpp"

namespace toaster::asset
{
	class TST_ASSET_API MeshImporter
	{
	public:
		MeshImporter(render::MeshManager *p_mesh_manager);

		auto importStaticFromFile(const std::filesystem::path &p_path) -> render::StaticMeshHandle;

	private:
		NonOwningPtr<render::MeshManager> m_meshManager{nullptr};
	};
}
