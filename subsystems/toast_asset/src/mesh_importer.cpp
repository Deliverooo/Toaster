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

			materials.emplace_back(nullptr); // TODO:
		}

		if (!scene->HasMaterials())
			materials.emplace_back(nullptr);

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

	auto MeshImporter::asyncLoadStaticMeshFromFile(render::MeshManager *p_mesh_manager, render::StaticMeshHandle p_dst_mesh, const std::filesystem::path &p_path) -> void
	{
		m_pendingImports.emplace_back([this, p_mesh_manager, p_dst_mesh, p_path]()-> void
		{
			if (m_terminationRequested.load())
				return;

			auto &gpu_mesh{p_mesh_manager->getStaticMesh(p_dst_mesh)};
			gpu_mesh.state->store(render::EMeshState::eLoading);

			const auto cpu_mesh_data{importStaticMeshDataFromFile(p_path)};

			if (m_terminationRequested.load())
				return;

			p_mesh_manager->uploadStaticMeshData(p_dst_mesh, cpu_mesh_data.vertices, cpu_mesh_data.indices, cpu_mesh_data.submeshes);
		});
	}
}
