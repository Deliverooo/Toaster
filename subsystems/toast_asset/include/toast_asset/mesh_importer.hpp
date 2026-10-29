#pragma once

#include "toast_asset.hpp"

#include <filesystem>
#include <taskflow/taskflow.hpp>

#include "texture_importer.hpp"
#include "toast_render/mesh.hpp"
#include "toast_render/material.hpp"


namespace toaster::ast
{
	struct TST_ASSET_API RawSubmeshData // Use if you need to generate meshes using an algorithm, then create meshlets
	{
		Vector<rnd::StaticMeshVertex> vertices;
		Vector<uint32>                indices;
		uint32                        materialIndex;
	};

	auto TST_ASSET_API generate_geometry_data(const RawSubmeshData &p_submesh_data, Vector<rnd::StaticMeshVertex> &p_out_vertices, Vector<rnd::Meshlet> &p_out_meshlets,
											  Vector<uint32> &      p_out_meshlet_vertices, Vector<uint8> &        p_out_meshlet_triangles) -> void;

	auto TST_ASSET_API load_static_mesh_from_file(const std::filesystem::path& p_path, rnd::StaticMeshHandle p_dst_mesh) -> void;
}
