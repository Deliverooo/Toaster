#pragma once

#include <mutex>

#include "material.hpp"

namespace toaster::render
{

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
		uint32 indexOffset{0u};
		int32  vertexOffset{0u};
		uint32 indexCount{0u};

		MaterialHandle material{nullptr};
	};

	struct TST_RENDER_API StaticMesh
	{
		std::vector<Submesh> submeshes;

		gpu::alloc::VirtualAllocationHandle vertexBufferAllocation{nullptr};
		gpu::alloc::VirtualAllocationHandle indexBufferAllocation{nullptr};
	};

	TST_DECLARE_HANDLE(StaticMesh);

	class TST_RENDER_API MeshManager
	{
	public:
		MeshManager(uint64 p_max_static_mesh_vertices = 50u * 1028u * 1028u, uint64 p_max_static_mesh_indices = 30u * 1028u * 1028u);
		~MeshManager();

		[[nodiscard]] auto createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
											const std::vector<Submesh> &         p_submeshes) -> StaticMeshHandle;
		auto destroyStaticMesh(StaticMeshHandle p_handle) -> void;

		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) -> StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto tryStaticMesh(StaticMeshHandle p_handle) -> StaticMesh * { return m_staticMeshes.tryGet(p_handle); }
		[[nodiscard]] auto tryStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh * { return m_staticMeshes.tryGet(p_handle); }

		auto getStaticMeshVertexBuffer() const -> gpu::BufferHandle { return m_staticMeshVertexBuffer; }
		auto getStaticMeshIndexBuffer() const -> gpu::BufferHandle { return m_staticMeshIndexBuffer; }

	private:
		Pool<StaticMesh> m_staticMeshes;

		uint64 m_maxStaticMeshVertices{0u};
		uint64 m_maxStaticMeshIndices{0u};

		gpu::BufferHandle m_staticMeshVertexBuffer{nullptr};
		gpu::BufferHandle m_staticMeshIndexBuffer{nullptr};

		gpu::alloc::VirtualBlockHandle m_staticMeshVertexBufferBlock{nullptr};
		gpu::alloc::VirtualBlockHandle m_staticMeshIndexBufferBlock{nullptr};

		std::mutex m_mutex;
	};
}
