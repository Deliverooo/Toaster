#include "toast_asset/mesh_importer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <meshoptimizer.h>

#include "toast_gpu/upload.hpp"

static constexpr uint32 s_MeshImportFlags{
	aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes |
	aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights | aiProcess_ValidateDataStructure | aiProcess_GlobalScale | aiProcess_ImproveCacheLocality |
	aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices
};

namespace toaster::asset
{
	MeshImporter::MeshImporter(render::MeshManager *p_mesh_manager, render::MaterialManager *p_material_manager,
							   TextureImporter *    p_texture_importer) : m_textureImporter(p_texture_importer), m_meshManager(p_mesh_manager),
																		  m_materialManager(p_material_manager), m_textureManager(p_texture_importer->getTextureManager())
	{
	}

	MeshImporter::~MeshImporter()
	{
		m_terminationRequested.store(true);
		for (auto &import: m_pendingImports)
			import.join();
	}

	auto MeshImporter::importStaticMeshDataFromFile(const std::filesystem::path &p_path) const -> MeshImportData
	{
		Assimp::Importer importer{};
		const aiScene *  scene{importer.ReadFile(p_path.string(), s_MeshImportFlags)};
		TST_PERMA_ASSERT(scene);

		MeshImportData out_data{};

		struct RawSubmeshData
		{
			std::vector<render::StaticMeshVertex> vertices;
			std::vector<uint32>                   indices;
		};
		// Used because it is more efficient to group mesh data by material
		std::unordered_map<uint32, RawSubmeshData> raw_submesh_data; // Maps from material index to raw data

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
				vertex.normal   = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};

				if (mesh->HasTextureCoords(0))
					vertex.texCoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
				else
					vertex.texCoord = {0.0f, 0.0f};
			}

			data.indices.resize(data.indices.size() + mesh->mNumFaces * 3u);
			for (uint32 f{0u}; f < mesh->mNumFaces; ++f)
			{
				aiFace face{mesh->mFaces[f]};

				if (face.mNumIndices == 3u)
				{
					for (uint32 i{0u}; i < face.mNumIndices; ++i)
						data.indices[(f * face.mNumIndices) + i] = face.mIndices[i];
				}
				else
				{
					TST_PERMA_ASSERT(false); // I don't know how this would be possible
				}
			}
		}

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

			// Shrink the meshlet data to fit the final count
			local_meshlets.resize(meshlet_count);
			local_meshlet_vertices.resize(meshlet_count * maxVertices);
			local_meshlet_triangles.resize(meshlet_count * maxTriangles * 3u);

			uint32 global_vertex_offset{static_cast<uint32>(out_data.vertices.size())};
			out_data.vertices.insert(out_data.vertices.end(), optimised_vertices.begin(), optimised_vertices.end());

			uint64 global_meshlet_offset{out_data.meshlets.size()};
			out_data.meshlets.resize(global_meshlet_offset + meshlet_count);

			uint64 global_meshlet_vertex_offset{out_data.meshletVertices.size()};
			uint64 global_meshlet_triangle_offset{out_data.meshletTriangles.size()};

			for (uint64 m{0u}; m < meshlet_count; ++m)
			{
				const meshopt_Meshlet &local_meshlet{local_meshlets[m]};

				meshopt_Bounds meshlet_bounds{
					meshopt_computeMeshletBounds(&local_meshlet_vertices[local_meshlet.vertex_offset], &local_meshlet_triangles[local_meshlet.triangle_offset],
												 local_meshlet.triangle_count, &out_data.vertices[global_vertex_offset].position.x, out_data.vertices.size(),
												 sizeof(render::StaticMeshVertex))
				};

				render::Meshlet &meshlet{out_data.meshlets[global_meshlet_offset + m]};
				meshlet.vertexOffset   = static_cast<uint32>(global_meshlet_vertex_offset) + local_meshlet.vertex_offset;
				meshlet.triangleOffset = static_cast<uint32>(global_meshlet_triangle_offset) + local_meshlet.triangle_offset;
				meshlet.vertexCount    = local_meshlet.vertex_count;
				meshlet.triangleCount  = local_meshlet.triangle_count;
				meshlet.materialIndex  = mat_idx;
				meshlet.boundingSphere = {meshlet_bounds.center[0u], meshlet_bounds.center[1u], meshlet_bounds.center[2u], meshlet_bounds.radius};
			}

			out_data.meshletVertices.insert(out_data.meshletVertices.end(), local_meshlet_vertices.begin(), local_meshlet_vertices.end());
			out_data.meshletTriangles.insert(out_data.meshletTriangles.end(), local_meshlet_triangles.begin(), local_meshlet_triangles.end());
		}

		for (uint32 i{0u}; i < scene->mNumMaterials; ++i)
		{
			const aiMaterial *ai_mat{scene->mMaterials[i]};

			aiColor3D ai_albedo_colour{};
			ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, ai_albedo_colour);

			auto &tst_mat{out_data.materials.emplace_back(m_materialManager->createMaterial())};
			m_materialManager->setAlbedoColour(tst_mat, {ai_albedo_colour.r, ai_albedo_colour.g, ai_albedo_colour.b});

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

			#define LOAD_TEXTURES 1
			if (has_albedo_map)
			{
				auto albedo_map_path{get_path_and_create_texture_if_exists(ai_albedo_map_path)};
				if (albedo_map_path.has_value())
				{
					#if LOAD_TEXTURES
					render::TextureHandle tex{m_textureManager->registerTexture()};
					m_textureImporter->asyncLoadTextureFromFile(tex, *albedo_map_path);
					m_materialManager->setAlbedoMap(tst_mat, tex);
					#endif
				}
				else
					m_materialManager->setAlbedoColour(tst_mat, {1.0f, 0.0f, 1.0f}); // Error magenta
			}

			aiString ai_normal_map_path;
			bool     has_normal_map{ai_mat->GetTexture(aiTextureType_NORMALS, 0, &ai_normal_map_path) == AI_SUCCESS};

			if (has_normal_map)
			{
				auto normal_map_path{get_path_and_create_texture_if_exists(ai_normal_map_path)};
				if (normal_map_path.has_value())
				{
					#if LOAD_TEXTURES
					render::TextureHandle tex{m_textureManager->registerTexture()};
					m_textureImporter->asyncLoadTextureFromFile(tex, *normal_map_path);
					m_materialManager->setNormalMap(tst_mat, tex);
					#endif
				}
			}
		}

		if (!scene->HasMaterials())
		{
			auto &tst_mat{out_data.materials.emplace_back(m_materialManager->createMaterial())};
			m_materialManager->setAlbedoColour(tst_mat, {1.0f, 0.0f, 1.0f});
		}

		return out_data;
	}

	auto MeshImporter::loadStaticMeshFromFile(render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path) const -> void
	{
		const auto cpu_mesh_data{importStaticMeshDataFromFile(p_path)};
		m_meshManager->uploadStaticMeshData(p_dst_mesh, cpu_mesh_data.vertices, cpu_mesh_data.meshlets, cpu_mesh_data.meshletVertices, cpu_mesh_data.meshletTriangles,
											cpu_mesh_data.materials);
	}

	auto MeshImporter::asyncLoadStaticMeshFromFile(render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path) -> void
	{
		m_pendingImports.emplace_back([this, p_dst_mesh, p_path]() -> void
		{
			const auto cpu_mesh_data{importStaticMeshDataFromFile(p_path)};

			if (m_terminationRequested.load())
				return;

			m_meshManager->uploadStaticMeshData(p_dst_mesh, cpu_mesh_data.vertices, cpu_mesh_data.meshlets, cpu_mesh_data.meshletVertices, cpu_mesh_data.meshletTriangles,
												cpu_mesh_data.materials);
		});
	}

	auto MeshImporter::waitImports() -> void
	{
		for (auto &import: m_pendingImports)
			import.join();

		m_pendingImports.clear();
	}
}
