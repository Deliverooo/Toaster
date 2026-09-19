#include "toast_asset/mesh_importer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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

	auto MeshImporter::importStaticMeshDataFromFile(const std::filesystem::path &p_path) -> MeshImportData
	{
		Assimp::Importer importer{};
		const aiScene *  scene{importer.ReadFile(p_path.string(), s_MeshImportFlags)};
		TST_PERMA_ASSERT(scene);

		MeshImportData out_data{};

		std::vector<render::MaterialHandle> materials;

		for (uint32 i{0u}; i < scene->mNumMaterials; ++i)
		{
			const aiMaterial *ai_mat{scene->mMaterials[i]};

			aiColor3D ai_albedo_colour{};
			ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, ai_albedo_colour);

			auto &tst_mat{materials.emplace_back(m_materialManager->createMaterial())};
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

			if (has_albedo_map)
			{
				auto albedo_map_path{get_path_and_create_texture_if_exists(ai_albedo_map_path)};
				if (albedo_map_path.has_value())
				{
					render::TextureHandle tex{m_textureManager->registerTexture()};
					m_textureImporter->asyncLoadTextureFromFile(tex, *albedo_map_path);

					m_materialManager->setAlbedoMap(tst_mat, tex);
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
					render::TextureHandle tex{m_textureManager->registerTexture()};
					m_textureImporter->asyncLoadTextureFromFile(tex, *normal_map_path);

					m_materialManager->setNormalMap(tst_mat, tex);
				}
			}
		}

		if (!scene->HasMaterials())
		{
			auto &tst_mat{materials.emplace_back(m_materialManager->createMaterial())};
			m_materialManager->setAlbedoColour(tst_mat, {1.0f, 0.0f, 1.0f});
		}

		for (uint32 m{0u}; m < scene->mNumMeshes; ++m)
		{
			const aiMesh *mesh{scene->mMeshes[m]};

			std::vector<render::StaticMeshVertex> mesh_vertices(mesh->mNumVertices);
			std::vector<uint32>                   mesh_indices(mesh->mNumFaces * 3u);

			auto &submesh{out_data.submeshes.emplace_back()};
			submesh.vertexOffset = static_cast<int32>(out_data.vertices.size());
			submesh.indexOffset  = out_data.indices.size();
			submesh.indexCount   = mesh->mNumFaces * 3u;
			submesh.material     = materials[mesh->mMaterialIndex]; // This is why I am loading the materials first

			for (int32 i{0u}; i < mesh->mNumVertices; ++i)
			{
				mesh_vertices[i].position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
				mesh_vertices[i].normal   = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

				if (mesh->HasTextureCoords(0))
					mesh_vertices[i].texCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
				else
					mesh_vertices[i].texCoord = {0.0f, 0.0f};
			}

			for (int32 i{0u}; i < mesh->mNumFaces; ++i)
			{
				aiFace face{mesh->mFaces[i]};
				for (int32 j{0u}; j < face.mNumIndices; ++j)
					mesh_indices[(i * face.mNumIndices) + j] = face.mIndices[j] + submesh.vertexOffset;
			}

			out_data.vertices.insert(out_data.vertices.end(), mesh_vertices.begin(), mesh_vertices.end());
			out_data.indices.insert(out_data.indices.end(), mesh_indices.begin(), mesh_indices.end());
		}
		return std::move(out_data);
	}

	auto MeshImporter::asyncLoadStaticMeshFromFile(render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path) -> void
	{
		m_meshManager->setStaticMeshState(p_dst_mesh, render::EMeshState::eLoading);
		m_pendingImports.emplace_back([this, p_dst_mesh, p_path]()-> void
		{
			const auto cpu_mesh_data{importStaticMeshDataFromFile(p_path)};

			if (m_terminationRequested.load())
				return;

			m_meshManager->uploadStaticMeshData(p_dst_mesh, cpu_mesh_data.vertices, cpu_mesh_data.indices, cpu_mesh_data.submeshes);
		});
	}

	auto MeshImporter::waitImports() -> void
	{
		for (auto &import: m_pendingImports)
			import.join();

		m_pendingImports.clear();
	}
}
