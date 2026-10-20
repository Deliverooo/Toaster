#pragma once

#include <filesystem>
#include <taskflow/taskflow.hpp>

#include "texture_importer.hpp"
#include "toast_render/mesh.hpp"
#include "toast_asset.hpp"
#include "toast_lib/filesystem.hpp"

namespace toaster::ast
{
	struct TST_ASSET_API RawSubmeshData // Use if you need to generate meshes using an algorithm, then create meshlets
	{
		std::vector<rnd::StaticMeshVertex> vertices;
		std::vector<uint32>                indices;
		uint32                             materialIndex;
	};

	auto TST_ASSET_API generateGeometryData(const RawSubmeshData &     p_submesh_data, std::vector<rnd::StaticMeshVertex> &p_out_vertices,
											std::vector<rnd::Meshlet> &p_out_meshlets, std::vector<uint32> &              p_out_meshlet_vertices,
											std::vector<uint8> &       p_out_meshlet_triangles) -> void;

	struct TST_ASSET_API TextureImportData
	{
		std::optional<std::filesystem::path> path;
		// TODO: Embedded textures with std::vector<uint8> data
	};

	struct TST_ASSET_API MaterialImportData // CPU-side material data to pass off to the GPU
	{
		TextureImportData albedoMap;
		TextureImportData normalMap;
		TextureImportData roughnessMap;
		TextureImportData metalnessMap;

		math::Float4 albedoColour;
		float32      roughness{0.0f};
		float32      metalness{0.0f};

		rnd::EMaterialFlags flags{rnd::EMaterialFlagBits::eBackfaceCulling};
	};

	struct TST_ASSET_API MeshImportData
	{
		std::vector<rnd::StaticMeshVertex> vertices;
		std::vector<rnd::Meshlet>          meshlets;
		std::vector<uint32>                meshletVertices;
		std::vector<uint8>                 meshletTriangles;
		std::vector<MaterialImportData>    materials;

		XMFLOAT4 boundingSphere;
	};

	class TST_ASSET_API MeshImporter
	{
	public:
		MeshImporter(rnd::MeshManager *p_mesh_manager, rnd::MaterialManager *p_material_manager, rnd::TextureManager *p_texture_manager);

		static auto importStaticMeshDataFromFile(const std::filesystem::path &p_path, RefPtr<MeshImportData> &p_out_data) -> void;

		// The callback will probably be used to set material properties just after loading. This does not mean that the GPU uploads have completed, only the mesh parsing has
		auto asyncLoadStaticMeshFromFile(rnd::StaticMeshHandle                             p_dst_mesh, const std::filesystem::path &p_path, tf::Executor &p_executor,
										 const std::function<void(rnd::StaticMeshHandle)> &p_on_finish_cb = nullptr) -> void;

	private:
		rnd::MeshManager *    m_meshManager{nullptr};
		rnd::MaterialManager *m_materialManager{nullptr};
		rnd::TextureManager * m_textureManager{nullptr};
	};
}
