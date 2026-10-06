#include "toast_asset/mesh_importer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <meshoptimizer.h>

#include "stb/stb_image.h"
#include "toast_gpu/upload.hpp"

static constexpr uint32 s_MeshImportFlags{
	aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes |
	aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights | aiProcess_ValidateDataStructure | aiProcess_GlobalScale | aiProcess_ImproveCacheLocality |
	aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices
};

namespace toaster::asset
{
	MeshImporter::MeshImporter(render::MeshManager *   p_mesh_manager, render::MaterialManager *p_material_manager,
							   render::TextureManager *p_texture_manager) : m_meshManager(p_mesh_manager), m_materialManager(p_material_manager),
																			m_textureManager(p_texture_manager)
	{
	}

	auto MeshImporter::importStaticMeshDataFromFile(const std::filesystem::path &p_path, RefPtr<MeshImportData> &p_out_data) -> void
	{
		Assimp::Importer importer{};
		const aiScene *  scene{importer.ReadFile(p_path.string(), s_MeshImportFlags)};
		TST_PERMA_ASSERT(scene);

		// MeshImportData out_data{};

		struct RawSubmeshData
		{
			std::vector<render::StaticMeshVertex> vertices;
			std::vector<uint32>                   indices;
		};
		// Used because it is more efficient to group mesh data by material
		std::unordered_map<uint32, RawSubmeshData> raw_submesh_data; // Maps from material index to raw data

		XMFLOAT3 min_bounds{FLT_MAX, FLT_MAX, FLT_MAX};
		XMFLOAT3 max_bounds{FLT_MIN, FLT_MIN, FLT_MIN};

		for (uint32 m{0u}; m < scene->mNumMeshes; ++m)
		{
			const aiMesh *mesh{scene->mMeshes[m]};

			uint32 material_index{mesh->mMaterialIndex};
			auto & data{raw_submesh_data[material_index]};

			uint32 base_vertex_offset{static_cast<uint32>(data.vertices.size())};
			data.vertices.resize(base_vertex_offset + mesh->mNumVertices);
			for (uint32 v{0u}; v < mesh->mNumVertices; ++v)
			{
				render::StaticMeshVertex &vertex{data.vertices[base_vertex_offset + v]};

				vertex.position = {mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z};

				min_bounds = {std::min(min_bounds.x, vertex.position.x), std::min(min_bounds.y, vertex.position.y), std::min(min_bounds.z, vertex.position.z)};
				max_bounds = {std::max(max_bounds.x, vertex.position.x), std::max(max_bounds.y, vertex.position.y), std::max(max_bounds.z, vertex.position.z)};

				vertex.normal = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};

				if (mesh->HasTextureCoords(0))
					vertex.texCoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
				else
					vertex.texCoord = {0.0f, 0.0f};
			}

			uint32 base_index_offset{static_cast<uint32>(data.indices.size())};
			data.indices.resize(data.indices.size() + mesh->mNumFaces * 3u);
			for (uint32 f{0u}; f < mesh->mNumFaces; ++f)
			{
				aiFace face{mesh->mFaces[f]};

				if (face.mNumIndices == 3u)
				{
					for (uint32 i{0u}; i < face.mNumIndices; ++i)
						data.indices[base_index_offset + (f * face.mNumIndices) + i] = base_vertex_offset + face.mIndices[i];
				}
				else
				{
					TST_PERMA_ASSERT(false); // I don't know how this would be possible
				}
			}
		}

		p_out_data->boundingSphere.x = (min_bounds.x + max_bounds.x) / 2.0f;
		p_out_data->boundingSphere.y = (min_bounds.y + max_bounds.y) / 2.0f;
		p_out_data->boundingSphere.z = (min_bounds.z + max_bounds.z) / 2.0f;

		float32 dx{max_bounds.x - p_out_data->boundingSphere.x};
		float32 dy{max_bounds.y - p_out_data->boundingSphere.y};
		float32 dz{max_bounds.z - p_out_data->boundingSphere.z};

		p_out_data->boundingSphere.w = std::sqrtf(dx * dx + dy * dy + dz * dz);

		std::println("{}", p_out_data->boundingSphere.w);

		static constexpr uint64  maxVertices{64u};
		static constexpr uint64  maxTriangles{128u};
		static constexpr float32 coneWeight{0.0f};

		for (auto &[mat_idx, data]: raw_submesh_data)
		{
			if (data.indices.empty())
				continue;

			std::vector<uint32> remap(data.indices.size());
			uint64              unique_vertex_count{
				meshopt_generateVertexRemap(remap.data(), data.indices.data(), data.indices.size(), data.vertices.data(), data.vertices.size(),
											sizeof(render::StaticMeshVertex))
			};

			std::vector<render::StaticMeshVertex> optimised_vertices(unique_vertex_count);
			std::vector<uint32>                   optimised_indices(data.indices.size());

			meshopt_remapIndexBuffer(optimised_indices.data(), data.indices.data(), data.indices.size(), remap.data());
			meshopt_remapVertexBuffer(optimised_vertices.data(), data.vertices.data(), data.vertices.size(), sizeof(render::StaticMeshVertex), remap.data());

			meshopt_optimizeVertexCache(optimised_indices.data(), optimised_indices.data(), optimised_indices.size(), optimised_vertices.size());
			meshopt_optimizeVertexFetch(optimised_vertices.data(), optimised_indices.data(), optimised_indices.size(), optimised_vertices.data(),
										optimised_vertices.size(), sizeof(render::StaticMeshVertex));

			uint64 max_meshlets{meshopt_buildMeshletsBound(optimised_indices.size(), maxVertices, maxTriangles)};

			std::vector<meshopt_Meshlet> local_meshlets(max_meshlets);
			std::vector<uint32>          local_meshlet_vertices(max_meshlets * maxVertices);
			std::vector<uint8>           local_meshlet_triangles(max_meshlets * maxTriangles * 3u);

			uint64 meshlet_count{
				meshopt_buildMeshlets(local_meshlets.data(), local_meshlet_vertices.data(), local_meshlet_triangles.data(), optimised_indices.data(),
									  optimised_indices.size(), &optimised_vertices[0].position.x, optimised_vertices.size(), sizeof(render::StaticMeshVertex),
									  maxVertices, maxTriangles, coneWeight)
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

			uint32 global_vertex_offset{static_cast<uint32>(p_out_data->vertices.size())};
			p_out_data->vertices.insert(p_out_data->vertices.end(), optimised_vertices.begin(), optimised_vertices.end());

			uint64 global_meshlet_offset{p_out_data->meshlets.size()};
			p_out_data->meshlets.resize(global_meshlet_offset + meshlet_count);

			uint64 global_meshlet_vertex_offset{p_out_data->meshletVertices.size()};
			uint64 global_meshlet_triangle_offset{p_out_data->meshletTriangles.size()};

			for (uint64 m{0u}; m < meshlet_count; ++m)
			{
				const meshopt_Meshlet &local_meshlet{local_meshlets[m]};

				meshopt_Bounds meshlet_bounds{
					meshopt_computeMeshletBounds(&local_meshlet_vertices[local_meshlet.vertex_offset], &local_meshlet_triangles[local_meshlet.triangle_offset],
												 local_meshlet.triangle_count, &p_out_data->vertices[global_vertex_offset].position.x, optimised_vertices.size(),
												 sizeof(render::StaticMeshVertex))
				};

				render::Meshlet &meshlet{p_out_data->meshlets[global_meshlet_offset + m]};
				meshlet.vertexOffset   = static_cast<uint32>(global_meshlet_vertex_offset) + local_meshlet.vertex_offset;
				meshlet.triangleOffset = static_cast<uint32>(global_meshlet_triangle_offset) + local_meshlet.triangle_offset;
				meshlet.vertexCount    = local_meshlet.vertex_count;
				meshlet.triangleCount  = local_meshlet.triangle_count;
				meshlet.materialIndex  = mat_idx;
				meshlet.boundingSphere = {meshlet_bounds.center[0u], meshlet_bounds.center[1u], meshlet_bounds.center[2u], meshlet_bounds.radius};
			}

			for (uint32 &vertex_index: local_meshlet_vertices)
				vertex_index += global_vertex_offset;

			p_out_data->meshletVertices.insert(p_out_data->meshletVertices.end(), local_meshlet_vertices.begin(), local_meshlet_vertices.end());
			p_out_data->meshletTriangles.insert(p_out_data->meshletTriangles.end(), local_meshlet_triangles.begin(), local_meshlet_triangles.end());
		}

		std::println("Material count: {}", scene->mNumMaterials);

		p_out_data->materials.resize(scene->mNumMaterials);
		for (uint32 i{0u}; i < scene->mNumMaterials; ++i)
		{
			const aiMaterial *ai_mat{scene->mMaterials[i]};

			MaterialImportData &tst_mat{p_out_data->materials[i]};

			aiColor3D ai_albedo_colour{};
			ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, ai_albedo_colour);

			tst_mat.albedoColour = {ai_albedo_colour.r, ai_albedo_colour.g, ai_albedo_colour.b, 1.0f};

			auto get_path_and_create_texture_if_exists{
				[p_path](const aiString &p_ai_path) -> std::optional<std::filesystem::path>
				{
					const std::filesystem::path texture_path{p_ai_path.C_Str()};
					std::filesystem::path       tex_map_path{std::filesystem::path{p_path}.parent_path() / texture_path};

					if (!std::filesystem::exists(tex_map_path))
					{
						tex_map_path = std::filesystem::path{p_path}.parent_path() / texture_path.filename();
						if (!std::filesystem::exists(tex_map_path))
						{
							return std::nullopt;
						}
					}
					return tex_map_path;
				}
			};

			aiString ai_albedo_map_path;
			bool     has_albedo_map{ai_mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &ai_albedo_map_path) == AI_SUCCESS};
			if (!has_albedo_map)
				has_albedo_map = ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &ai_albedo_map_path) == AI_SUCCESS;

			if (has_albedo_map)
			{
				auto albedo_map_path{get_path_and_create_texture_if_exists(ai_albedo_map_path)};
				tst_mat.albedoMap.path = albedo_map_path;
			}

			aiString ai_normal_map_path;
			bool     has_normal_map{ai_mat->GetTexture(aiTextureType_NORMALS, 0, &ai_normal_map_path) == AI_SUCCESS};

			if (has_normal_map)
			{
				auto normal_map_path{get_path_and_create_texture_if_exists(ai_normal_map_path)};
				if (normal_map_path.has_value())
					tst_mat.normalMap.path = *normal_map_path;
			}
		}

		if (!scene->HasMaterials())
		{
			auto &default_mat{p_out_data->materials.emplace_back()};
			default_mat.albedoColour = {1.0f, 0.0f, 1.0f, 1.0f};
		}
	}

	auto MeshImporter::asyncLoadStaticMeshFromFile(render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path, tf::Executor &p_executor,
												   const std::function<void(render::StaticMeshHandle)> &p_on_finish_cb) -> void
	{
		p_executor.silent_async([this, p_dst_mesh, p_path, &p_executor, p_on_finish_cb]()-> void
		{
			tf::Taskflow graph{};

			RefPtr<MeshImportData> import_data{makeReference<MeshImportData>()};

			tf::Task load_mesh_task{
				graph.emplace([import_data, p_path]() mutable -> void
				{
					importStaticMeshDataFromFile(p_path, import_data);
				})
			};

			auto     materials{makeReference<std::vector<render::MaterialHandle> >()};
			tf::Task create_materials_task{
				graph.emplace([this, materials, import_data, &p_executor]() mutable -> void
				{
					materials->resize(import_data->materials.size());
					for (uint32 i{0u}; i < import_data->materials.size(); ++i)
					{
						const MaterialImportData &cpu_mat{import_data->materials[i]};
						render::MaterialHandle &  gpu_mat{materials->at(i)};

						gpu_mat = m_materialManager->createMaterial();
						m_materialManager->setAlbedoColour(gpu_mat, cpu_mat.albedoColour);
						if (cpu_mat.albedoMap.path.has_value())
						{
							render::TextureHandle albedo_map{m_textureManager->registerTexture()};
							asyncLoadTextureFromFile(albedo_map, *m_textureManager, *cpu_mat.albedoMap.path, p_executor);
							m_materialManager->setAlbedoMap(gpu_mat, albedo_map);
						}
						// else
						// m_materialManager->setAlbedoColour(gpu_mat, {1.0f, 0.0f, 1.0f}); // TODO: Embed

						if (cpu_mat.normalMap.path.has_value())
						{
							render::TextureHandle normal_map{m_textureManager->registerTexture()};
							asyncLoadTextureFromFile(normal_map, *m_textureManager, *cpu_mat.normalMap.path, p_executor);
							m_materialManager->setNormalMap(gpu_mat, normal_map);
						}
					}
				})
			};

			tf::Task upload_task{
				graph.emplace([this, import_data, p_dst_mesh, materials]() -> void
				{
					m_meshManager->uploadStaticMeshData(p_dst_mesh, import_data->vertices, import_data->meshlets, import_data->meshletVertices,
														import_data->meshletTriangles, *materials, XMLoadFloat4(&import_data->boundingSphere));
				})
			};

			create_materials_task.succeed(load_mesh_task);
			create_materials_task.precede(upload_task);

			load_mesh_task.precede(upload_task);

			p_executor.run(graph).wait();

			if (p_on_finish_cb)
				p_on_finish_cb(p_dst_mesh);
		});
	}
}
