#pragma once

#include <mutex>
#include <shared_mutex>

#include "material.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	struct TST_RENDER_API StaticMeshVertex
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT2 texCoord;
	};

	struct TST_RENDER_API Meshlet
	{
		uint32 materialIndex{0u}; // Index into the per-mesh material array

		uint32 vertexOffset{0u};
		uint32 triangleOffset{0u};
		uint32 vertexCount{0u};
		uint32 triangleCount{0u};

		tsm::float4 boundingSphere{0.0f};
	};

	struct TST_RENDER_API StaticMesh
	{
		std::vector<Meshlet>        meshlets;
		std::vector<MaterialHandle> materials; // The actual materials the meshlets hold indices to

		gpu::alloc::PageAllocation vertexBufferAllocation{};
		gpu::alloc::PageAllocation meshletBufferAllocation{};
		gpu::alloc::PageAllocation meshletVertexBufferAllocation{};
		gpu::alloc::PageAllocation meshletTriangleBufferAllocation{};
		gpu::alloc::PageAllocation materialIndirectionBufferAllocation{};

		gpu::upload::StateTrackerHandle stateTracker{nullptr};

		[[nodiscard]] auto vertexBufferOffset() const -> uint64 { return vertexBufferAllocation.offset / sizeof(StaticMeshVertex); }
		[[nodiscard]] auto meshletBufferOffset() const -> uint64 { return meshletBufferAllocation.offset / sizeof(Meshlet); }
		[[nodiscard]] auto meshletVertexBufferOffset() const -> uint64 { return meshletVertexBufferAllocation.offset / sizeof(uint32); }
		[[nodiscard]] auto meshletTriangleBufferOffset() const -> uint64 { return meshletTriangleBufferAllocation.offset / sizeof(uint8); }
		[[nodiscard]] auto materialIndirectionBufferOffset() const -> uint64 { return materialIndirectionBufferAllocation.offset / sizeof(uint32); }
	};

	TST_DECLARE_HANDLE(StaticMesh);

	class TST_RENDER_API MeshManager
	{
	public:
		MeshManager(RenderContext *p_render_ctx);
		~MeshManager();

		// Register so you can upload the gpu data once it is loaded from disk
		[[nodiscard]] auto registerStaticMesh() -> StaticMeshHandle;
		auto               uploadStaticMeshData(StaticMeshHandle     p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<Meshlet> &p_meshlets,
												const std::vector<uint32> &        p_meshlet_vertices, const std::vector<uint8> & p_meshlet_triangles,
												const std::vector<MaterialHandle> &p_materials) -> void;

		[[nodiscard]] auto createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<Meshlet> &      p_meshlets,
											const std::vector<uint32> &          p_meshlet_vertices, const std::vector<uint8> &p_meshlet_triangles,
											const std::vector<MaterialHandle> &  p_materials) -> StaticMeshHandle;
		auto destroyStaticMesh(StaticMeshHandle p_handle) -> void;

		auto isStaticMeshReady(StaticMeshHandle p_handle) -> bool;

		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) -> StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) -> StaticMesh * { return m_staticMeshes.tryGet(p_handle); }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh * { return m_staticMeshes.tryGet(p_handle); }

	private:
		// Total initial page size of 576 Mib. Unless you are loading the entire planet, pages won't really be added during runtime.
		// For scale, the Sponza only uses ~60 Mib for its vertex data.
		static constexpr uint64 vertexPageSize{256u * 1024u * 1024u};          // 256 Mib
		static constexpr uint64 meshletPageSize{64u * 1024u * 1024u};          // 64 Mib.
		static constexpr uint64 meshletVertexPageSize{128u * 1024u * 1024u};   // 128 Mib
		static constexpr uint64 meshletTrianglePageSize{128u * 1024u * 1024u}; // 128 Mib

		NonOwningPtr<RenderContext> m_renderCtx{nullptr};

		std::mutex                              m_pageMutex; // Because all of these operations are called at the same time, I only need one mutex.
		UniquePtr<gpu::alloc::GPUPageAllocator> m_vertexPager{nullptr};
		UniquePtr<gpu::alloc::GPUPageAllocator> m_meshletPager{nullptr};
		UniquePtr<gpu::alloc::GPUPageAllocator> m_meshletVertexPager{nullptr};
		UniquePtr<gpu::alloc::GPUPageAllocator> m_meshletTrianglePager{nullptr};

		// This is more than enough. E.g. Sponza only uses 27-30 materials, which is 30 * sizeof(uint32) = 120 bytes
		static constexpr uint64                 materialIndirectionPageSize{2u * 1024 * 1024u}; // 2 Mib.
		UniquePtr<gpu::alloc::GPUPageAllocator> m_materialIndirectionPager{nullptr};

		Pool<StaticMesh> m_staticMeshes;

		#ifndef NDEBUG
		std::mutex m_debugMutex; // Used for print statement synchronisation
		#endif
	};
}
