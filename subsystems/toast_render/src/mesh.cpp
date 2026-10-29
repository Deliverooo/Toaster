#include "toast_render/mesh.hpp"

#include <algorithm>
#include <ranges>

#include "toast_gpu/api.hpp"
#include "toast_gpu/upload.hpp"
#include "toast_lib/format.hpp"
#include "toast_lib/pool.hpp"
#include "toast_render/render_context.hpp"

namespace toaster::rnd
{
	struct MeshManager
	{
		// Total initial page size of 576 Mib. Unless you are loading the entire planet, pages won't really be added during runtime.
		// For scale, the Sponza only uses ~60 Mib for its vertex data.
		static constexpr uint64 vertexPageSize{256u * 1024u * 1024u};          // 256 Mib
		static constexpr uint64 meshletPageSize{64u * 1024u * 1024u};          // 64 Mib.
		static constexpr uint64 meshletVertexPageSize{128u * 1024u * 1024u};   // 128 Mib
		static constexpr uint64 meshletTrianglePageSize{128u * 1024u * 1024u}; // 128 Mib

		RenderContext *rndCtx{nullptr};

		mtx_t pageMtx; // Because all of these operations are called at the same time, I only need one mutex.

		gpu::PageAllocator vertexPager;
		gpu::PageAllocator meshletPager;
		gpu::PageAllocator meshletVertexPager;
		gpu::PageAllocator meshletTrianglePager;

		// This is more than enough. E.g. Sponza only uses 27-30 materials, which is 30 * sizeof(uint32) = 120 bytes
		static constexpr uint64 materialIndirectionBufferSize{1000u * 1000u}; // (1,000,000 materials)
		gpu::BufferHandle       materialIndirectionBuffer{nullptr};
		gpu::VirtualBlockHandle materialIndirectionBlock{nullptr};

		gpu::BufferHandle staticMeshMetadataBuffer{nullptr}; // Indexed by the mesh handle's id

		Pool<StaticMesh> staticMeshes;
	};

	auto mesh_mngr_create(RenderContext *p_render_ctx) -> MeshManager *
	{
		auto mngr{new MeshManager{}};

		mngr->rndCtx = p_render_ctx;

		mtx_init(&mngr->pageMtx, mtx_plain);

		gpu::GPUContext *                       gpu_ctx{p_render_ctx->gpuCtx};
		gpu::AllocationContext *                alloc_ctx{p_render_ctx->allocCtx};
		const gpu::ResourceDescriptorHeapHandle resource_heap{p_render_ctx->resourceHeap};

		gpu::page_allocator_create(alloc_ctx, &mngr->vertexPager, MeshManager::vertexPageSize, resource_heap);
		gpu::page_allocator_create(alloc_ctx, &mngr->meshletPager, MeshManager::meshletPageSize, resource_heap);
		gpu::page_allocator_create(alloc_ctx, &mngr->meshletVertexPager, MeshManager::meshletVertexPageSize, resource_heap);
		gpu::page_allocator_create(alloc_ctx, &mngr->meshletTrianglePager, MeshManager::meshletTrianglePageSize, resource_heap);

		gpu::BufferDesc material_indirection_buffer_desc{};
		material_indirection_buffer_desc.size       = MeshManager::materialIndirectionBufferSize * sizeof(uint32);
		material_indirection_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eTransferDst | gpu::EBufferUsageFlagBits::eStorageBuffer;
		material_indirection_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;
		mngr->materialIndirectionBuffer             = gpu::buffer_create(gpu_ctx, material_indirection_buffer_desc);
		mngr->materialIndirectionBlock              = gpu::virtual_block_create(alloc_ctx, MeshManager::materialIndirectionBufferSize * sizeof(uint32));

		gpu::BufferDesc static_mesh_metadata_buffer_desc{};
		static_mesh_metadata_buffer_desc.size       = 1028u * 1028u * sizeof(StaticMeshMetadata); // 28 Mib / ~ 1 million meshes
		static_mesh_metadata_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eTransferDst | gpu::EBufferUsageFlagBits::eStorageBuffer;
		static_mesh_metadata_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal; // More efficient?
		mngr->staticMeshMetadataBuffer              = gpu::buffer_create(gpu_ctx, static_mesh_metadata_buffer_desc);

		mngr->staticMeshes.setDestructorUserData(mngr);
		mngr->staticMeshes.setDestructorFn(+[](StaticMesh *p_data, void *p_user_data) -> void
		{
			const auto mngr{static_cast<MeshManager *>(p_user_data)};

			p_data->meshlets.clear();

			// TODO: Freeing these allocations should be deferred (while under the mutex lock in the local deferred scope)
			mtx_lock(&mngr->pageMtx);

			// If you just used static_mesh_register, you won't have allocated anything.
			if (p_data->vertexBufferAllocation.allocation)
				gpu::page_allocator_free_page_allocation(mngr->rndCtx->allocCtx, &mngr->vertexPager, &p_data->vertexBufferAllocation);
			if (p_data->meshletBufferAllocation.allocation)
				gpu::page_allocator_free_page_allocation(mngr->rndCtx->allocCtx, &mngr->meshletPager, &p_data->meshletBufferAllocation);
			if (p_data->meshletVertexBufferAllocation.allocation)
				gpu::page_allocator_free_page_allocation(mngr->rndCtx->allocCtx, &mngr->meshletVertexPager, &p_data->meshletVertexBufferAllocation);
			if (p_data->meshletTriangleBufferAllocation.allocation)
				gpu::page_allocator_free_page_allocation(mngr->rndCtx->allocCtx, &mngr->meshletTrianglePager, &p_data->meshletTriangleBufferAllocation);
			if (p_data->materialIndirectionBufferAllocation)
				gpu::virtual_block_free(mngr->rndCtx->allocCtx, p_data->materialIndirectionBufferAllocation);
			
			p_data->materialIndirectionBufferAllocation = nullptr;
			p_data->materialIndirectionBufferOffset     = 0u;

			mtx_unlock(&mngr->pageMtx);
		});

		return mngr;
	}

	auto mesh_mngr_destroy(MeshManager *p_mngr) -> void
	{
		gpu::GPUContext *       gpu_ctx{p_mngr->rndCtx->gpuCtx};
		gpu::AllocationContext *alloc_ctx{p_mngr->rndCtx->allocCtx};
		gpu::UploadContext *    upload_ctx{p_mngr->rndCtx->uploadCtx};

		gpu::wait_idle(gpu_ctx); // Make sure all the state trackers are ready to be destroyed
		gpu::poll_uploads(upload_ctx);

		p_mngr->staticMeshes.clear();

		gpu::buffer_destroy(gpu_ctx, p_mngr->staticMeshMetadataBuffer);
		gpu::buffer_destroy(gpu_ctx, p_mngr->materialIndirectionBuffer);
		gpu::virtual_block_destroy(alloc_ctx, p_mngr->materialIndirectionBlock);

		gpu::page_allocator_destroy(alloc_ctx, &p_mngr->vertexPager);
		gpu::page_allocator_destroy(alloc_ctx, &p_mngr->meshletPager);
		gpu::page_allocator_destroy(alloc_ctx, &p_mngr->meshletVertexPager);
		gpu::page_allocator_destroy(alloc_ctx, &p_mngr->meshletTrianglePager);

		mtx_destroy(&p_mngr->pageMtx);

		delete p_mngr;
	}

	auto mesh_mngr_get_static_mesh_metadata_buffer_address(const MeshManager *p_mngr) -> gpu::DeviceAddress
	{
		return gpu::buffer_get_address(p_mngr->rndCtx->gpuCtx, p_mngr->staticMeshMetadataBuffer);
	}

	auto mesh_mngr_get_material_indirection_buffer_address(const MeshManager *p_mngr) -> gpu::DeviceAddress
	{
		return gpu::buffer_get_address(p_mngr->rndCtx->gpuCtx, p_mngr->materialIndirectionBuffer);
	}

	auto static_mesh_register(MeshManager *p_mngr) -> StaticMeshHandle
	{
		gpu::UploadContext *upload_ctx{p_mngr->rndCtx->uploadCtx};

		StaticMesh temp_mesh{};

		// Vertex buffer, meshlet buffer, meshlet vertex buffer, meshlet triangle buffer, material indirection buffer and metadata buffer
		temp_mesh.stateTracker = gpu::state_tracker_register(upload_ctx, 6u);

		return p_mngr->staticMeshes.emplace(std::move(temp_mesh));
	}

	auto static_mesh_upload_data(MeshManager *p_mngr, StaticMeshHandle p_static_mesh, const StaticMeshGeometryData &p_geometry_data) -> void
	{
		gpu::AllocationContext *alloc_ctx{p_mngr->rndCtx->allocCtx};
		gpu::UploadContext *    upload_ctx{p_mngr->rndCtx->uploadCtx};

		StaticMesh &static_mesh{p_mngr->staticMeshes[p_static_mesh]};

		static_mesh.meshlets.assign_range(p_geometry_data.meshlets);
		static_mesh.materials.assign_range(p_geometry_data.materials);

		static_mesh.boundingSphere = p_geometry_data.boundingSphere;

		const uint64 vertex_buffer_size{p_geometry_data.vertices.size_bytes()};
		const uint64 meshlet_buffer_size{p_geometry_data.meshlets.size_bytes()};
		const uint64 meshlet_vertex_buffer_size{p_geometry_data.meshletVertices.size_bytes()};
		const uint64 meshlet_triangle_buffer_size{p_geometry_data.meshletIndices.size_bytes()};
		const uint64 material_indirection_buffer_buffer_size{p_geometry_data.materials.size_bytes()};

		mtx_lock(&p_mngr->pageMtx);

		gpu::page_allocator_allocate_across_pages(alloc_ctx, &p_mngr->vertexPager, vertex_buffer_size, alignof(StaticMeshVertex), &static_mesh.vertexBufferAllocation);
		gpu::page_allocator_allocate_across_pages(alloc_ctx, &p_mngr->meshletPager, meshlet_buffer_size, alignof(Meshlet), &static_mesh.meshletBufferAllocation);
		gpu::page_allocator_allocate_across_pages(alloc_ctx, &p_mngr->meshletVertexPager, meshlet_vertex_buffer_size, alignof(uint32),
												  &static_mesh.meshletVertexBufferAllocation);
		gpu::page_allocator_allocate_across_pages(alloc_ctx, &p_mngr->meshletTrianglePager, meshlet_triangle_buffer_size, alignof(uint8),
												  &static_mesh.meshletTriangleBufferAllocation);

		static_mesh.materialIndirectionBufferAllocation = gpu::virtual_block_allocate(alloc_ctx, p_mngr->materialIndirectionBlock,
																					  material_indirection_buffer_buffer_size, alignof(uint32));
		static_mesh.materialIndirectionBufferOffset = gpu::virtual_allocation_get_offset(alloc_ctx, static_mesh.materialIndirectionBufferAllocation);

		mtx_unlock(&p_mngr->pageMtx);

		gpu::upload_data_to_buffer(upload_ctx, gpu::BufferUploadDesc{
									   static_mesh.vertexBufferAllocation.buffer,
									   p_geometry_data.vertices.data(),
									   vertex_buffer_size,
									   static_mesh.vertexBufferAllocation.offset
								   }, static_mesh.stateTracker);

		gpu::upload_data_to_buffer(upload_ctx, gpu::BufferUploadDesc{
									   static_mesh.meshletBufferAllocation.buffer,
									   p_geometry_data.meshlets.data(),
									   meshlet_buffer_size,
									   static_mesh.meshletBufferAllocation.offset
								   }, static_mesh.stateTracker);

		gpu::upload_data_to_buffer(upload_ctx, gpu::BufferUploadDesc{
									   static_mesh.meshletVertexBufferAllocation.buffer,
									   p_geometry_data.meshletVertices.data(),
									   meshlet_vertex_buffer_size,
									   static_mesh.meshletVertexBufferAllocation.offset
								   }, static_mesh.stateTracker);

		gpu::upload_data_to_buffer(upload_ctx, gpu::BufferUploadDesc{
									   static_mesh.meshletTriangleBufferAllocation.buffer,
									   p_geometry_data.meshletIndices.data(),
									   meshlet_triangle_buffer_size,
									   static_mesh.meshletTriangleBufferAllocation.offset
								   }, static_mesh.stateTracker);

		// The material ids are just the indices into the pool and also the gpu buffer
		const Vector<uint32> material_indirection_buffer_data{
			static_mesh.materials | std::views::transform([](const MaterialHandle p_handle) -> uint32 { return p_handle.getId(); }) | std::ranges::to<Vector<uint32> >()
		};

		gpu::upload_data_to_buffer(upload_ctx, gpu::BufferUploadDesc{
									   p_mngr->materialIndirectionBuffer,
									   material_indirection_buffer_data.data(),
									   material_indirection_buffer_buffer_size,
									   static_mesh.materialIndirectionBufferOffset
								   }, static_mesh.stateTracker);

		StaticMeshMetadata metadata{};
		metadata.boundingSphere = p_geometry_data.boundingSphere;
		metadata.meshletCount   = static_cast<uint32>(p_geometry_data.meshlets.size());

		metadata.vertexBufferOffset              = static_cast<uint32>(static_mesh.vertexBufferOffset());
		metadata.meshletBufferOffset             = static_cast<uint32>(static_mesh.meshletBufferOffset());
		metadata.meshletVertexBufferOffset       = static_cast<uint32>(static_mesh.meshletVertexBufferOffset());
		metadata.meshletTriangleBufferOffset     = static_cast<uint32>(static_mesh.meshletTriangleBufferOffset());
		metadata.materialIndirectionBufferOffset = static_cast<uint32>(static_mesh.materialIndirectionBufferOffset / sizeof(uint32));

		metadata.vertexBufferPageSlot          = static_cast<uint8>(static_mesh.vertexBufferAllocation.heapSlot);
		metadata.meshletBufferPageSlot         = static_cast<uint8>(static_mesh.meshletBufferAllocation.heapSlot);
		metadata.meshletVertexBufferPageSlot   = static_cast<uint8>(static_mesh.meshletVertexBufferAllocation.heapSlot);
		metadata.meshletTriangleBufferPageSlot = static_cast<uint8>(static_mesh.meshletTriangleBufferAllocation.heapSlot);

		uint64 metadata_offset{p_static_mesh.getId() * sizeof(StaticMeshMetadata)};

		gpu::upload_data_to_buffer(upload_ctx, gpu::BufferUploadDesc{p_mngr->staticMeshMetadataBuffer, &metadata, sizeof(StaticMeshMetadata), metadata_offset},
								   static_mesh.stateTracker);
	}

	auto static_mesh_create(MeshManager *p_mngr, const StaticMeshGeometryData &p_geometry_data) -> StaticMeshHandle
	{
		const StaticMeshHandle out_handle{static_mesh_register(p_mngr)};
		static_mesh_upload_data(p_mngr, out_handle, p_geometry_data);
		return out_handle;
	}

	auto static_mesh_destroy(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> void
	{
		p_mngr->staticMeshes.destroy(p_static_mesh);
	}

	auto static_mesh_is_ready(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> bool
	{
		const StaticMesh &mesh{p_mngr->staticMeshes[p_static_mesh]};
		return gpu::state_tracker_is_ready(p_mngr->rndCtx->uploadCtx, mesh.stateTracker);
	}

	auto static_mesh_get(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> const StaticMesh *
	{
		return &p_mngr->staticMeshes[p_static_mesh];
	}

	auto static_mesh_get(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> StaticMesh *
	{
		return &p_mngr->staticMeshes[p_static_mesh];
	}

	auto static_mesh_try_get(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> const StaticMesh *
	{
		return p_mngr->staticMeshes.tryGet(p_static_mesh);
	}

	auto static_mesh_try_get(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> StaticMesh *
	{
		return p_mngr->staticMeshes.tryGet(p_static_mesh);
	}
}
