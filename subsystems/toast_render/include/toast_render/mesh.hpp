#pragma once

#include <mutex>

#include "material.hpp"

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

		EMeshState state{EMeshState::eUnloaded};
		uint64     vertexReadyToken{0u};
		uint64     indexReadyToken{0u};
	};

	TST_DECLARE_HANDLE(StaticMesh);

	class TST_RENDER_API MeshManager
	{
	public:
		MeshManager(uint64 p_max_static_mesh_vertices = 50u * 1028u * 1028u, uint64 p_max_static_mesh_indices = 30u * 1028u * 1028u);
		~MeshManager();

		// Register so you can upload the gpu data once it is loaded from disk
		[[nodiscard]] auto registerStaticMesh() -> StaticMeshHandle;
		auto               uploadStaticMeshData(StaticMeshHandle p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
												const std::vector<Submesh> &   p_submeshes) -> void;

		[[nodiscard]] auto createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
											const std::vector<Submesh> &         p_submeshes) -> StaticMeshHandle;
		auto destroyStaticMesh(StaticMeshHandle p_handle) -> void;

		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) -> StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) -> StaticMesh * { return m_staticMeshes.tryGet(p_handle); }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh * { return m_staticMeshes.tryGet(p_handle); }

		// Thread safe operations
		auto getStaticMeshState(StaticMeshHandle p_handle) -> EMeshState;
		auto setStaticMeshState(StaticMeshHandle p_handle, EMeshState p_state) -> void;

		struct StaticMeshThreadData
		{
			std::vector<Submesh> submeshes;

			uint64 vertexBufferOffset{0u};
			uint64 vertexBufferSize{0u};

			uint64 indexBufferOffset{0u};
			uint64 indexBufferSize{0u};

			EMeshState state{EMeshState::eUnloaded};
		};

		// Returns a copy of the static mesh's data, so the operation can be done under the internal mutex's lock
		auto getStaticMeshThreadData(StaticMeshHandle p_handle) -> StaticMeshThreadData;
		auto tryGetStaticMeshThreadData(StaticMeshHandle p_handle) -> std::optional<StaticMeshThreadData>;

		auto getStaticMeshVertexBuffer() const -> gpu::BufferHandle { return m_staticMeshVertexBuffer; }
		auto getStaticMeshIndexBuffer() const -> gpu::BufferHandle { return m_staticMeshIndexBuffer; }

		auto pollMeshUploads() -> void;

	private:
		Pool<StaticMesh> m_staticMeshes;

		uint64 m_maxStaticMeshVertices{0u};
		uint64 m_maxStaticMeshIndices{0u};

		gpu::BufferHandle m_staticMeshVertexBuffer{nullptr};
		gpu::BufferHandle m_staticMeshIndexBuffer{nullptr};

		gpu::alloc::VirtualBlockHandle m_staticMeshVertexBufferBlock{nullptr};
		gpu::alloc::VirtualBlockHandle m_staticMeshIndexBufferBlock{nullptr};

		std::unordered_set<StaticMeshHandle> m_pendingMeshUploads;

		std::mutex m_mutex;
	};
}
