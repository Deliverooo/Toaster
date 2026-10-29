#include "toast_asset/mesh_importer.hpp"

#include <meshoptimizer.h>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#include "../../../cmake-build-release-visual-studio/_deps/fastgltf-src/include/fastgltf/tools.hpp"

namespace toaster::ast
{
	auto generate_geometry_data(const RawSubmeshData &p_submesh_data, Vector<rnd::StaticMeshVertex> &p_out_vertices, Vector<rnd::Meshlet> &p_out_meshlets,
								Vector<uint32> &      p_out_meshlet_vertices, Vector<uint8> &        p_out_meshlet_triangles) -> void
	{
		static constexpr uint64  maxVertices{64u};
		static constexpr uint64  maxTriangles{128u};
		static constexpr float32 coneWeight{0.0f};

		Vector<uint32> remap(p_submesh_data.indices.size());
		uint64         unique_vertex_count{
			meshopt_generateVertexRemap(remap.data(), p_submesh_data.indices.data(), p_submesh_data.indices.size(), p_submesh_data.vertices.data(),
										p_submesh_data.vertices.size(), sizeof(rnd::StaticMeshVertex))
		};

		Vector<rnd::StaticMeshVertex> optimised_vertices(unique_vertex_count);
		Vector<uint32>                optimised_indices(p_submesh_data.indices.size());

		meshopt_remapIndexBuffer(optimised_indices.data(), p_submesh_data.indices.data(), p_submesh_data.indices.size(), remap.data());
		meshopt_remapVertexBuffer(optimised_vertices.data(), p_submesh_data.vertices.data(), p_submesh_data.vertices.size(), sizeof(rnd::StaticMeshVertex), remap.data());

		meshopt_optimizeVertexCache(optimised_indices.data(), optimised_indices.data(), optimised_indices.size(), optimised_vertices.size());
		meshopt_optimizeVertexFetch(optimised_vertices.data(), optimised_indices.data(), optimised_indices.size(), optimised_vertices.data(), optimised_vertices.size(),
									sizeof(rnd::StaticMeshVertex));

		uint64 max_meshlets{meshopt_buildMeshletsBound(optimised_indices.size(), maxVertices, maxTriangles)};

		Vector<meshopt_Meshlet> local_meshlets(max_meshlets);
		Vector<uint32>          local_meshlet_vertices(max_meshlets * maxVertices);
		Vector<uint8>           local_meshlet_triangles(max_meshlets * maxTriangles * 3u);

		uint64 meshlet_count{
			meshopt_buildMeshlets(local_meshlets.data(), local_meshlet_vertices.data(), local_meshlet_triangles.data(), optimised_indices.data(),
								  optimised_indices.size(), &optimised_vertices[0].position.x, optimised_vertices.size(), sizeof(rnd::StaticMeshVertex), maxVertices,
								  maxTriangles, coneWeight)
		};

		// Shrink the meshlet data to the actual packed counts.
		local_meshlets.resize(meshlet_count);
		uint64 meshlet_vertex_count{0u};
		uint64 meshlet_triangle_count{0u};
		for (const meshopt_Meshlet &meshlet: local_meshlets)
		{
			meshlet_vertex_count   = std::max(meshlet_vertex_count, uint64{meshlet.vertex_offset} + meshlet.vertex_count);
			meshlet_triangle_count = std::max(meshlet_triangle_count, uint64{meshlet.triangle_offset} + meshlet.triangle_count * 3u);
		}
		local_meshlet_vertices.resize(meshlet_vertex_count);
		local_meshlet_triangles.resize(meshlet_triangle_count);

		uint32 global_vertex_offset{static_cast<uint32>(p_out_vertices.size())};
		p_out_vertices.insert(p_out_vertices.end(), optimised_vertices.begin(), optimised_vertices.end());

		uint64 global_meshlet_offset{p_out_meshlets.size()};
		p_out_meshlets.resize(global_meshlet_offset + meshlet_count);

		uint64 global_meshlet_vertex_offset{p_out_meshlet_vertices.size()};
		uint64 global_meshlet_triangle_offset{p_out_meshlet_triangles.size()};

		for (uint64 m{0u}; m < meshlet_count; ++m)
		{
			const meshopt_Meshlet &local_meshlet{local_meshlets[m]};

			meshopt_Bounds meshlet_bounds{
				meshopt_computeMeshletBounds(&local_meshlet_vertices[local_meshlet.vertex_offset], &local_meshlet_triangles[local_meshlet.triangle_offset],
											 local_meshlet.triangle_count, &p_out_vertices[global_vertex_offset].position.x, optimised_vertices.size(),
											 sizeof(rnd::StaticMeshVertex))
			};

			rnd::Meshlet &meshlet{p_out_meshlets[global_meshlet_offset + m]};
			meshlet.vertexOffset   = static_cast<uint32>(global_meshlet_vertex_offset) + local_meshlet.vertex_offset;
			meshlet.triangleOffset = static_cast<uint32>(global_meshlet_triangle_offset) + local_meshlet.triangle_offset;
			meshlet.vertexCount    = local_meshlet.vertex_count;
			meshlet.triangleCount  = local_meshlet.triangle_count;
			meshlet.materialIndex  = p_submesh_data.materialIndex;
			meshlet.boundingSphere = {meshlet_bounds.center[0u], meshlet_bounds.center[1u], meshlet_bounds.center[2u], meshlet_bounds.radius};
		}

		for (uint32 &vertex_index: local_meshlet_vertices)
			vertex_index += global_vertex_offset;

		p_out_meshlet_vertices.insert(p_out_meshlet_vertices.end(), local_meshlet_vertices.begin(), local_meshlet_vertices.end());
		p_out_meshlet_triangles.insert(p_out_meshlet_triangles.end(), local_meshlet_triangles.begin(), local_meshlet_triangles.end());
	}

	auto load_static_mesh_from_file(const std::filesystem::path &p_path, rnd::StaticMeshHandle p_dst_mesh) -> void
	{
		constexpr fastgltf::Options c_options{fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers};

		auto gltf_file{fastgltf::GltfDataBuffer::FromPath(p_path)};
		if (gltf_file.error() != fastgltf::Error::None)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to load GLTF file!");
			return;
		}
		fastgltf::Parser parser;

		auto asset_result{parser.loadGltf(gltf_file.get(), p_path.parent_path(), c_options)};
		if (asset_result.error() != fastgltf::Error::None)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to load GLTF file!");
			return;
		}

		fastgltf::Asset &asset{asset_result.get()};

		const uint64 scene_index{asset.defaultScene.value_or(0ull)};

		// fastgltf::iterateSceneNodes(asset, scene_index, fastgltf::math::fmat4x4(), [&](fastgltf::Node &p_node, fastgltf::math::fmat4x4 p_transform) -> void
		// {
		// 	if (p_node.meshIndex.has_value())
		// 	{
		// 	}
		// });
	}
}
