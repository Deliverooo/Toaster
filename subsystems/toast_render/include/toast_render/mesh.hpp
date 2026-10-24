#pragma once

#include <mutex>

#include "material.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::rnd
{
	struct TST_RENDER_API StaticMeshVertex
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 bitangent;
		XMFLOAT2 texCoord;
	};

	struct TST_RENDER_API Meshlet
	{
		uint32 materialIndex{0u}; // Index into the per-mesh material array

		uint32 vertexOffset{0u};
		uint32 triangleOffset{0u};
		uint32 vertexCount{0u};
		uint32 triangleCount{0u};

		XMFLOAT4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};
	};

	struct TST_RENDER_API StaticMeshMetadata
	{
		XMFLOAT4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};

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
		std::vector<Meshlet>        meshlets;
		std::vector<MaterialHandle> materials; // The actual materials the meshlets hold indices to

		gpu::PageAllocation          vertexBufferAllocation{};
		gpu::PageAllocation          meshletBufferAllocation{};
		gpu::PageAllocation          meshletVertexBufferAllocation{};
		gpu::PageAllocation          meshletTriangleBufferAllocation{};
		gpu::VirtualAllocationHandle materialIndirectionBufferAllocation{nullptr};
		uint32                       materialIndirectionBufferOffset{0u};

		gpu::StateTrackerHandle stateTracker{nullptr};

		XMFLOAT4 boundingSphere{0.0f, 0.0f, 0.0f, 1.0f};

		[[nodiscard]] auto vertexBufferOffset() const -> uint64 { return vertexBufferAllocation.offset / sizeof(StaticMeshVertex); }
		[[nodiscard]] auto meshletBufferOffset() const -> uint64 { return meshletBufferAllocation.offset / sizeof(Meshlet); }
		[[nodiscard]] auto meshletVertexBufferOffset() const -> uint64 { return meshletVertexBufferAllocation.offset / sizeof(uint32); }
		[[nodiscard]] auto meshletTriangleBufferOffset() const -> uint64 { return meshletTriangleBufferAllocation.offset / sizeof(uint8); }
	};

	struct TST_RENDER_API Mesh
	{
	};

	TST_DECLARE_HANDLE(StaticMesh);

	class TST_RENDER_API MeshManager
	{
	public:
		MeshManager(RenderContext *p_render_ctx);
		~MeshManager();

		// Register so you can upload the gpu data once it is loaded from disk
		[[nodiscard]] auto registerStaticMesh() -> StaticMeshHandle;
		auto XM_CALLCONV   uploadStaticMeshData(StaticMeshHandle p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<Meshlet> &p_meshlets,
												const std::vector<uint32> &p_meshlet_vertices, const std::vector<uint8> &p_meshlet_triangles,
												const std::vector<MaterialHandle> &p_materials, XMVECTOR p_bounding_sphere) -> void;

		[[nodiscard]] auto XM_CALLCONV createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<Meshlet> &p_meshlets,
														const std::vector<uint32> &          p_meshlet_vertices, const std::vector<uint8> &p_meshlet_triangles,
														const std::vector<MaterialHandle> &  p_materials, XMVECTOR p_bounding_sphere) -> StaticMeshHandle;
		auto destroyStaticMesh(StaticMeshHandle p_handle) -> void;

		auto isStaticMeshReady(StaticMeshHandle p_handle) const -> bool;

		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) -> StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto getStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh & { return m_staticMeshes[p_handle]; }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) -> StaticMesh * { return m_staticMeshes.tryGet(p_handle); }
		[[nodiscard]] auto tryGetStaticMesh(StaticMeshHandle p_handle) const -> const StaticMesh * { return m_staticMeshes.tryGet(p_handle); }

		[[nodiscard]] auto getStaticMeshMetadataBufferAddress() const -> gpu::DeviceAddress
		{
			return gpu::buffer_get_address(m_renderCtx->gpuCtx, m_staticMeshMetadataBuffer);
		}

		[[nodiscard]] auto getMaterialIndirectionBufferAddress() const -> gpu::DeviceAddress
		{
			return gpu::buffer_get_address(m_renderCtx->gpuCtx, m_materialIndirectionBuffer);
		}

		auto getTotalVertexAllocatedSize() const -> uint64 { return gpu::page_allocator_get_total_allocated_size(m_renderCtx->allocCtx, &m_vertexPager); }
		auto getTotalMeshletAllocatedSize() const -> uint64 { return gpu::page_allocator_get_total_allocated_size(m_renderCtx->allocCtx, &m_meshletPager); }
		auto getTotalMeshletVertexAllocatedSize() const -> uint64 { return gpu::page_allocator_get_total_allocated_size(m_renderCtx->allocCtx, &m_meshletVertexPager); }

		auto getTotalMeshletTriangleAllocatedSize() const -> uint64
		{
			return gpu::page_allocator_get_total_allocated_size(m_renderCtx->allocCtx, &m_meshletTrianglePager);
		}

	private:
		// Total initial page size of 576 Mib. Unless you are loading the entire planet, pages won't really be added during runtime.
		// For scale, the Sponza only uses ~60 Mib for its vertex data.
		static constexpr uint64 vertexPageSize{256u * 1024u * 1024u};          // 256 Mib
		static constexpr uint64 meshletPageSize{64u * 1024u * 1024u};          // 64 Mib.
		static constexpr uint64 meshletVertexPageSize{128u * 1024u * 1024u};   // 128 Mib
		static constexpr uint64 meshletTrianglePageSize{128u * 1024u * 1024u}; // 128 Mib

		RenderContext *m_renderCtx{nullptr};

		std::mutex m_pageMutex; // Because all of these operations are called at the same time, I only need one mutex.

		gpu::PageAllocator m_vertexPager;
		gpu::PageAllocator m_meshletPager;
		gpu::PageAllocator m_meshletVertexPager;
		gpu::PageAllocator m_meshletTrianglePager;

		static constexpr uint64 materialIndirectionBufferSize{1000u * 1000u}; // (1,000,000 materials)
		gpu::BufferHandle       m_materialIndirectionBuffer{nullptr};
		gpu::VirtualBlockHandle m_materialIndirectionBlock{nullptr};
		// This is more than enough. E.g. Sponza only uses 27-30 materials, which is 30 * sizeof(uint32) = 120 bytes
		// gpu::PageAllocator      m_materialIndirectionPager;

		gpu::BufferHandle m_staticMeshMetadataBuffer{nullptr}; // Indexed by the mesh handle's id

		Pool<StaticMesh> m_staticMeshes;

		#ifndef NDEBUG
		std::mutex m_debugMutex; // Used for print statement synchronisation
		#endif
	};
}
