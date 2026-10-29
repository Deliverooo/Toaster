#pragma once

#include <span>
#include <vector>

#include "toast_render.hpp"
#include "toast_gpu/allocation.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::rnd
{
	struct RenderContext;
	struct Material;
	TST_DECLARE_HANDLE(Material);

	struct TST_RENDER_API StaticMeshVertex
	{
		math::Float3 position;
		math::Float3 normal;
		math::Float3 tangent;
		math::Float3 bitangent;
		math::Float2 texCoord;
	};

	struct TST_RENDER_API Meshlet
	{
		uint32 materialIndex{0u}; // Index into the per-mesh material array

		uint32 vertexOffset{0u};
		uint32 triangleOffset{0u};
		uint32 vertexCount{0u};
		uint32 triangleCount{0u};

		math::Float4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};
	};

	struct TST_RENDER_API StaticMeshMetadata
	{
		math::Float4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};

		uint32 meshletCount{0u}; // Used for populating the indirect dispatch command in compute culling shaders

		uint32 vertexBufferOffset{0u};
		uint32 meshletBufferOffset{0u};
		uint32 meshletVertexBufferOffset{0u};
		uint32 meshletTriangleBufferOffset{0u};
		uint32 materialIndirectionBufferOffset{0u};

		uint8 vertexBufferPageSlot{0u};
		uint8 meshletBufferPageSlot{0u};
		uint8 meshletVertexBufferPageSlot{0u};
		uint8 meshletTriangleBufferPageSlot{0u};
	};

	struct TST_RENDER_API StaticMesh
	{
		Vector<Meshlet>        meshlets;
		Vector<MaterialHandle> materials; // The actual materials the meshlets hold indices to

		gpu::PageAllocation          vertexBufferAllocation{};
		gpu::PageAllocation          meshletBufferAllocation{};
		gpu::PageAllocation          meshletVertexBufferAllocation{};
		gpu::PageAllocation          meshletTriangleBufferAllocation{};
		gpu::VirtualAllocationHandle materialIndirectionBufferAllocation{nullptr};
		uint32                       materialIndirectionBufferOffset{0u};

		gpu::StateTrackerHandle stateTracker{nullptr};

		math::Float4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};

		[[nodiscard]] auto vertexBufferOffset() const -> uint64 { return vertexBufferAllocation.offset / sizeof(StaticMeshVertex); }
		[[nodiscard]] auto meshletBufferOffset() const -> uint64 { return meshletBufferAllocation.offset / sizeof(Meshlet); }
		[[nodiscard]] auto meshletVertexBufferOffset() const -> uint64 { return meshletVertexBufferAllocation.offset / sizeof(uint32); }
		[[nodiscard]] auto meshletTriangleBufferOffset() const -> uint64 { return meshletTriangleBufferAllocation.offset / sizeof(uint8); }
	};

	struct TST_RENDER_API Mesh
	{
	};

	TST_DECLARE_HANDLE(StaticMesh);

	struct MeshManager;

	auto TST_RENDER_API mesh_mngr_create(RenderContext *p_render_ctx) -> MeshManager *;
	auto TST_RENDER_API mesh_mngr_destroy(MeshManager *p_mngr) -> void;
	auto TST_RENDER_API mesh_mngr_get_static_mesh_metadata_buffer_address(const MeshManager *p_mngr) -> gpu::DeviceAddress;
	auto TST_RENDER_API mesh_mngr_get_material_indirection_buffer_address(const MeshManager *p_mngr) -> gpu::DeviceAddress;

	#pragma region static mesh

	// Used for the upload and create function
	struct TST_RENDER_API StaticMeshGeometryData
	{
		std::span<const StaticMeshVertex> vertices;
		std::span<const Meshlet>          meshlets;
		std::span<const uint32>           meshletVertices;
		std::span<const uint8>            meshletIndices;
		std::span<const MaterialHandle>   materials;

		math::Float4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};
	};

	auto TST_RENDER_API static_mesh_register(MeshManager *p_mngr) -> StaticMeshHandle;
	auto TST_RENDER_API static_mesh_upload_data(MeshManager *p_mngr, StaticMeshHandle p_static_mesh, const StaticMeshGeometryData &p_geometry_data) -> void;
	auto TST_RENDER_API static_mesh_create(MeshManager *p_mngr, const StaticMeshGeometryData &p_geometry_data) -> StaticMeshHandle;
	auto TST_RENDER_API static_mesh_destroy(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> void;

	auto TST_RENDER_API static_mesh_is_ready(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> bool;

	auto TST_RENDER_API static_mesh_get(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> const StaticMesh *;
	auto TST_RENDER_API static_mesh_get(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> StaticMesh *;
	auto TST_RENDER_API static_mesh_try_get(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> const StaticMesh *;
	auto TST_RENDER_API static_mesh_try_get(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> StaticMesh *;

	#pragma endregion
}
