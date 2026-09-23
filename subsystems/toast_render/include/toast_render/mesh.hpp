#pragma once

#include <mutex>
#include <shared_mutex>

#include "material.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	struct TST_RENDER_API alignas(16u) DefaultMaterial
	{
		XMFLOAT3 albedoColour{1.0f, 1.0f, 1.0f};
		uint32   albedoMap{UINT32_MAX};
	};

	enum class EMeshState : uint8
	{
		eUnloaded,
		eLoading,
		eUploadingToGPU,
		eReady
	};

	struct TST_RENDER_API StaticMeshVertex
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 bitangent;
		XMFLOAT2 texCoord;
	};

	struct TST_RENDER_API Submesh
	{
		MaterialHandle material{nullptr};
		uint32         indexOffset{0u};
		int32          vertexOffset{0u};
		uint32         indexCount{0u};
	};

	struct TST_RENDER_API StaticMesh
	{
		std::vector<Submesh> submeshes;

		gpu::alloc::VirtualAllocationHandle vertexBufferAllocation{nullptr};
		gpu::alloc::VirtualAllocationHandle indexBufferAllocation{nullptr};

		// Useful to cache in the data struct
		uint64 vertexBufferOffset{0u};
		uint64 indexBufferOffset{0u};

		gpu::upload::StateTrackerHandle stateTracker{nullptr};
	};

	TST_DECLARE_HANDLE(StaticMesh);

	class TST_RENDER_API MeshManager
	{
	public:
		MeshManager(uint64 p_max_static_mesh_vertices = 20u * 1000u * 1000u, uint64 p_max_static_mesh_indices = 30u * 1000u * 1000u);
		~MeshManager();

		// Register so you can upload the gpu data once it is loaded from disk
		[[nodiscard]] auto registerStaticMesh() -> StaticMeshHandle;
		auto               uploadStaticMeshData(StaticMeshHandle p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
												const std::vector<Submesh> &   p_submeshes) -> void;

		[[nodiscard]] auto createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
											const std::vector<Submesh> &         p_submeshes) -> StaticMeshHandle;
		auto destroyStaticMesh(StaticMeshHandle p_handle) -> void;

		auto isStaticMeshReady(StaticMeshHandle p_handle) -> bool;

		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) -> StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) -> StaticMesh * { return m_staticMeshes.tryGet(p_handle); }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh * { return m_staticMeshes.tryGet(p_handle); }

		[[nodiscard]] auto getStaticMeshVertexBuffer() const -> gpu::BufferHandle { return m_staticMeshVertexBuffer; }
		[[nodiscard]] auto getStaticMeshIndexBuffer() const -> gpu::BufferHandle { return m_staticMeshIndexBuffer; }

	private:
		Pool<StaticMesh> m_staticMeshes;

		uint64 m_maxStaticMeshVertices{0u};
		uint64 m_maxStaticMeshIndices{0u};

		gpu::BufferHandle m_staticMeshVertexBuffer{nullptr};
		gpu::BufferHandle m_staticMeshIndexBuffer{nullptr};

		gpu::alloc::VirtualBlockHandle m_staticMeshVertexBufferBlock{nullptr};
		gpu::alloc::VirtualBlockHandle m_staticMeshIndexBufferBlock{nullptr};
	};
}
