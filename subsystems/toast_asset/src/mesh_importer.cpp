#include "toast_asset/mesh_importer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

static constexpr uint32 s_MeshImportFlags{
	aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes |
	aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights | aiProcess_ValidateDataStructure | aiProcess_GlobalScale | aiProcess_ImproveCacheLocality |
	aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices
};

namespace toaster::asset
{
	MeshImporter::MeshImporter(render::MeshManager *p_mesh_manager) : m_meshManager(p_mesh_manager)
	{
	}

	auto MeshImporter::importStaticFromFile(const std::filesystem::path &p_path) -> render::StaticMeshHandle
	{
		Assimp::Importer importer{};
		const aiScene *  scene{importer.ReadFile(p_path.string(), s_MeshImportFlags)};
		TST_PERMA_ASSERT(scene);

		std::vector<render::Submesh>        submeshes;
		std::vector<render::MaterialHandle> materials;

		for (uint32 i{0u}; i < scene->mNumMaterials; ++i)
		{
			const aiMaterial *ai_mat{scene->mMaterials[i]};

			materials.emplace_back(nullptr); // TODO:
		}

		if (!scene->HasMaterials())
			materials.emplace_back(nullptr);

		std::vector<render::StaticMeshVertex> vertices;
		std::vector<uint32>                   indices;

		for (uint32 m{0u}; m < scene->mNumMeshes; ++m)
		{
			const aiMesh *mesh{scene->mMeshes[m]};

			std::vector<render::StaticMeshVertex> mesh_vertices(mesh->mNumVertices);
			std::vector<uint32>                   mesh_indices(mesh->mNumFaces * 3u);

			auto &submesh{submeshes.emplace_back()};
			submesh.vertexOffset = static_cast<int32>(vertices.size());
			submesh.indexOffset  = indices.size();
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

			vertices.insert(vertices.end(), mesh_vertices.begin(), mesh_vertices.end());
			indices.insert(indices.end(), mesh_indices.begin(), mesh_indices.end());
		}

		return m_meshManager->createStaticMesh(vertices, indices, submeshes);
	}
}
