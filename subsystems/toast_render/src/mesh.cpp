#include "toast_render/mesh.hpp"

#include <algorithm>

#include "toast_gpu/upload.hpp"
#include "toast_lib/format.hpp"

namespace toaster::render
{
	MeshManager::MeshManager(RenderContext *p_render_ctx) : m_renderCtx(p_render_ctx)
	{
		m_vertexPager              = makeUnique<gpu::alloc::GPUPageAllocator>(vertexPageSize, m_renderCtx->getResourceHeap());
		m_meshletPager             = makeUnique<gpu::alloc::GPUPageAllocator>(meshletPageSize, m_renderCtx->getResourceHeap());
		m_meshletVertexPager       = makeUnique<gpu::alloc::GPUPageAllocator>(meshletVertexPageSize, m_renderCtx->getResourceHeap());
		m_meshletTrianglePager     = makeUnique<gpu::alloc::GPUPageAllocator>(meshletTrianglePageSize, m_renderCtx->getResourceHeap());
		m_materialIndirectionPager = makeUnique<gpu::alloc::GPUPageAllocator>(materialIndirectionPageSize, m_renderCtx->getResourceHeap());

		gpu::BufferDesc static_mesh_metadata_buffer_desc{};
		static_mesh_metadata_buffer_desc.size       = 1028u * 1028u * sizeof(StaticMeshMetadata); // 28 Mib / ~ 1 million meshes
		static_mesh_metadata_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eTransferDst | gpu::EBufferUsageFlagBits::eStorageBuffer;
		static_mesh_metadata_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal; // More efficient?
		m_staticMeshMetadataBuffer                  = gpu::createBuffer(static_mesh_metadata_buffer_desc);

		m_staticMeshes.setDestructorUserData(this);
		m_staticMeshes.setDestructorFn(+[](StaticMesh *p_data, void *p_user_data) -> void
		{
			auto ts{static_cast<MeshManager *>(p_user_data)};

			p_data->meshlets.clear();

			{
				std::scoped_lock<std::mutex> lock{ts->m_pageMutex};

				ts->m_vertexPager->freePageAllocation(p_data->vertexBufferAllocation);
				ts->m_meshletPager->freePageAllocation(p_data->meshletBufferAllocation);
				ts->m_meshletVertexPager->freePageAllocation(p_data->meshletVertexBufferAllocation);
				ts->m_meshletTrianglePager->freePageAllocation(p_data->meshletTriangleBufferAllocation);
				ts->m_materialIndirectionPager->freePageAllocation(p_data->materialIndirectionBufferAllocation);
			}

			p_data->vertexBufferAllocation              = {};
			p_data->meshletBufferAllocation             = {};
			p_data->meshletVertexBufferAllocation       = {};
			p_data->meshletTriangleBufferAllocation     = {};
			p_data->materialIndirectionBufferAllocation = {};

			gpu::upload::destroyStateTracker(p_data->stateTracker);
		});
	}

	MeshManager::~MeshManager()
	{
		gpu::waitIdle(); // Make sure all the state trackers are ready to be destroyed
		gpu::upload::pollUploads();

		m_staticMeshes.clear();

		gpu::destroyBuffer(m_staticMeshMetadataBuffer);

		m_vertexPager.reset();
		m_meshletPager.reset();
		m_meshletVertexPager.reset();
		m_meshletTrianglePager.reset();
		m_materialIndirectionPager.reset();
	}

	auto MeshManager::registerStaticMesh() -> StaticMeshHandle
	{
		StaticMesh temp_mesh{};

		// Vertex buffer, meshlet buffer, meshlet vertex buffer, meshlet triangle buffer, material indirection buffer and metadata buffer
		temp_mesh.stateTracker = gpu::upload::createStateTracker(6u);

		return m_staticMeshes.emplace(std::move(temp_mesh));
	}

	auto MeshManager::uploadStaticMeshData(StaticMeshHandle p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<Meshlet> &p_meshlets,
										   const std::vector<uint32> &p_meshlet_vertices, const std::vector<uint8> &p_meshlet_triangles,
										   const std::vector<MaterialHandle> &p_materials, XMVECTOR p_bounding_sphere) -> void
	{
		StaticMesh &static_mesh{m_staticMeshes[p_handle]};

		static_mesh.meshlets  = p_meshlets;
		static_mesh.materials = p_materials;

		XMStoreFloat4(&static_mesh.boundingSphere, p_bounding_sphere);

		const uint64 vertex_buffer_size{p_vertices.size() * sizeof(StaticMeshVertex)};
		const uint64 meshlet_buffer_size{p_meshlets.size() * sizeof(Meshlet)};
		const uint64 meshlet_vertex_buffer_size{p_meshlet_vertices.size() * sizeof(uint32)};
		const uint64 meshlet_triangle_buffer_size{p_meshlet_triangles.size() * sizeof(uint8)};
		const uint64 material_indirection_buffer_buffer_size{p_materials.size() * sizeof(uint32)};

		{
			TST_SCP_LOCK(m_pageMutex);

			m_vertexPager->allocateAcrossPages(vertex_buffer_size, alignof(StaticMeshVertex), static_mesh.vertexBufferAllocation);
			m_meshletPager->allocateAcrossPages(meshlet_buffer_size, alignof(Meshlet), static_mesh.meshletBufferAllocation);
			m_meshletVertexPager->allocateAcrossPages(meshlet_vertex_buffer_size, alignof(uint32), static_mesh.meshletVertexBufferAllocation);
			m_meshletTrianglePager->allocateAcrossPages(meshlet_triangle_buffer_size, alignof(uint8), static_mesh.meshletTriangleBufferAllocation);
			m_materialIndirectionPager->allocateAcrossPages(material_indirection_buffer_buffer_size, alignof(uint32), static_mesh.materialIndirectionBufferAllocation);
		}

		#ifndef NDEBUG
		std::scoped_lock<std::mutex> print_lock{m_debugMutex};

		std::println("Allocating and uploading mesh data: [Id: {} | Mag: {}]", p_handle.getId(), p_handle.getMagic());
		#endif

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.vertexBufferAllocation.buffer,
											p_vertices.data(),
											vertex_buffer_size,
											static_mesh.vertexBufferAllocation.offset
										}, static_mesh.stateTracker);
		#ifndef NDEBUG
		std::println("\tVertex buffer: Offset: {} | Size: {}", static_mesh.vertexBufferAllocation.offset, ByteSize{vertex_buffer_size});
		#endif

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.meshletBufferAllocation.buffer,
											p_meshlets.data(),
											meshlet_buffer_size,
											static_mesh.meshletBufferAllocation.offset
										}, static_mesh.stateTracker);
		#ifndef NDEBUG
		std::println("\tMeshlet buffer: Offset: {} | Size: {}", static_mesh.meshletBufferAllocation.offset, ByteSize{meshlet_buffer_size});
		#endif

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.meshletVertexBufferAllocation.buffer,
											p_meshlet_vertices.data(),
											meshlet_vertex_buffer_size,
											static_mesh.meshletVertexBufferAllocation.offset
										}, static_mesh.stateTracker);
		#ifndef NDEBUG
		std::println("\tMeshlet vertex buffer: Offset: {} | Size: {}", static_mesh.meshletVertexBufferAllocation.offset, ByteSize{meshlet_vertex_buffer_size});
		#endif

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.meshletTriangleBufferAllocation.buffer,
											p_meshlet_triangles.data(),
											meshlet_triangle_buffer_size,
											static_mesh.meshletTriangleBufferAllocation.offset
										}, static_mesh.stateTracker);
		#ifndef NDEBUG
		std::println("\tMeshlet triangle buffer: Offset: {} | Size: {}", static_mesh.meshletTriangleBufferAllocation.offset, ByteSize{meshlet_triangle_buffer_size});
		#endif

		// The material ids are just the indices into the pool and also the gpu buffer
		const std::vector<uint32> material_indirection_buffer_data{
			p_materials | std::views::transform([](const MaterialHandle p_handle) -> uint32 { return p_handle.getId(); }) | std::ranges::to<std::vector>()
		};

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.materialIndirectionBufferAllocation.buffer,
											material_indirection_buffer_data.data(),
											material_indirection_buffer_buffer_size,
											static_mesh.materialIndirectionBufferAllocation.offset
										}, static_mesh.stateTracker);
		#ifndef NDEBUG
		std::println("\tMaterial indirection buffer: Offset: {} | Size: {}", static_mesh.materialIndirectionBufferAllocation.offset,
					 ByteSize{material_indirection_buffer_buffer_size});
		#endif

		#ifndef NDEBUG
		std::println("\tUploading mesh metadata\n");
		#endif

		StaticMeshMetadata metadata{};

		XMStoreFloat4(&metadata.boundingSphere, p_bounding_sphere);

		metadata.meshletCount = static_cast<uint32>(p_meshlets.size());

		metadata.vertexBufferOffset              = static_cast<uint32>(static_mesh.vertexBufferOffset());
		metadata.meshletBufferOffset             = static_cast<uint32>(static_mesh.meshletBufferOffset());
		metadata.meshletVertexBufferOffset       = static_cast<uint32>(static_mesh.meshletVertexBufferOffset());
		metadata.meshletTriangleBufferOffset     = static_cast<uint32>(static_mesh.meshletTriangleBufferOffset());
		metadata.materialIndirectionBufferOffset = static_cast<uint32>(static_mesh.materialIndirectionBufferOffset());

		metadata.vertexBufferPageSlot              = static_cast<uint8>(static_mesh.vertexBufferAllocation.heapSlot);
		metadata.meshletBufferPageSlot             = static_cast<uint8>(static_mesh.meshletBufferAllocation.heapSlot);
		metadata.meshletVertexBufferPageSlot       = static_cast<uint8>(static_mesh.meshletVertexBufferAllocation.heapSlot);
		metadata.meshletTriangleBufferPageSlot     = static_cast<uint8>(static_mesh.meshletTriangleBufferAllocation.heapSlot);
		metadata.materialIndirectionBufferPageSlot = static_cast<uint8>(static_mesh.materialIndirectionBufferAllocation.heapSlot);

		uint64 metadata_offset{p_handle.getId() * sizeof(StaticMeshMetadata)};

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{m_staticMeshMetadataBuffer, &metadata, sizeof(StaticMeshMetadata), metadata_offset},
										static_mesh.stateTracker);
	}

	auto MeshManager::createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<Meshlet> &      p_meshlets,
									   const std::vector<uint32> &          p_meshlet_vertices, const std::vector<uint8> &p_meshlet_triangles,
									   const std::vector<MaterialHandle> &  p_materials, XMVECTOR                         p_bounding_sphere) -> StaticMeshHandle
	{
		const StaticMeshHandle out_handle{registerStaticMesh()};
		uploadStaticMeshData(out_handle, p_vertices, p_meshlets, p_meshlet_vertices, p_meshlet_triangles, p_materials, p_bounding_sphere);
		return out_handle;
	}

	auto MeshManager::destroyStaticMesh(StaticMeshHandle p_handle) -> void
	{
		m_staticMeshes.destroy(p_handle);
	}

	auto MeshManager::isStaticMeshReady(StaticMeshHandle p_handle) -> bool
	{
		StaticMesh &mesh{m_staticMeshes[p_handle]};
		return gpu::upload::isStateTrackerReady(mesh.stateTracker);
	}
}
