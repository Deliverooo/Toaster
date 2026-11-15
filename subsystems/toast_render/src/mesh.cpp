#include "toast_render/mesh.hpp"

#include <algorithm>
#include <ranges>

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/api.hpp"
#include "toast_gpu/upload.hpp"
#include "toast_gpu/frame.hpp"

#include "toast_lib/atomic.hpp"
#include "toast_lib/freelist_allocator.hpp"
#include "toast_lib/pool.hpp"

namespace toaster::rnd
{
	struct Page
	{
		gpu::BufferHandle        buffer{nullptr};
		gpu::VirtualBlockHandle  block{nullptr};
		uint64                   size{0ull};
		uint32                   heapSlot{UINT32_MAX};
		uint32                   id{0u};
		TST_ALIGN_ATOMIC(uint32) activeAllocations{0u};
	};

	struct PageAllocator
	{
		FreelistAllocator<uint32> pageIdAllocator{};
		Vector<Page>              pages;
		uint64                    pageSize{0ull};
		uint32                    activePageId{0u};
	};

	struct MeshManager
	{
		// Total initial page size of 576 Mib. Unless you are loading the entire planet, pages won't really be added during runtime.
		// For scale, the Sponza only uses ~60 Mib for its vertex data.
		static constexpr uint64 vertexPageSize{256u * 1024u * 1024u};          // 256 Mib
		static constexpr uint64 meshletPageSize{64u * 1024u * 1024u};          // 64 Mib
		static constexpr uint64 meshletVertexPageSize{128u * 1024u * 1024u};   // 128 Mib
		static constexpr uint64 meshletTrianglePageSize{128u * 1024u * 1024u}; // 128 Mib

		MeshManagerDesc desc;

		mtx_t pageMtx; // Because all of these operations are called at the same time, I only need one mutex.

		PageAllocator vertexPager;
		PageAllocator meshletPager;
		PageAllocator meshletVertexPager;
		PageAllocator meshletTrianglePager;

		// This is more than enough. E.g. Sponza only uses 27-30 materials, which is 30 * sizeof(uint32) = 120 bytes
		static constexpr uint64 materialIndirectionBufferSize{1000u * 1000u}; // (1,000,000 materials)
		gpu::BufferHandle       materialIndirectionBuffer{nullptr};
		gpu::VirtualBlockHandle materialIndirectionBlock{nullptr};

		gpu::BufferHandle staticMeshMetadataBuffer{nullptr}; // Indexed by the mesh handle's id

		FixedPool<StaticMesh, 32u, 32u> staticMeshes; // 1024
	};

	auto _page_allocator_create_page(MeshManager *p_mngr, uint64 p_size, uint32 p_id, Page *p_out_page) -> void
	{
		p_out_page->size = p_size;
		p_out_page->id   = p_id;

		gpu::BufferDesc page_desc{};
		page_desc.size       = p_size;
		page_desc.memoryType = gpu::EMemoryType::eDeviceLocal;
		page_desc.usage      = gpu::EBufferUsageFlagBits::eTransferDst | gpu::EBufferUsageFlagBits::eStorageBuffer;
		p_out_page->buffer   = gpu::buffer_create(p_mngr->desc.gpuCtx, page_desc);
		p_out_page->block    = gpu::virtual_block_create(p_mngr->desc.allocCtx, p_size);
		p_out_page->heapSlot = gpu::resource_descriptor_heap_alloc_buffer_slot(p_mngr->desc.gpuCtx, p_mngr->desc.resourceHeap);
		resource_descriptor_heap_write_buffer(p_mngr->desc.gpuCtx, p_mngr->desc.resourceHeap, p_out_page->heapSlot, p_out_page->buffer);
	}

	auto _page_allocator_insert_page(MeshManager *p_mngr, PageAllocator *p_allocator) -> void
	{
		const uint32 page_id{p_allocator->pageIdAllocator.alloc_slot()};

		Page &page{p_allocator->pages.emplace_back()};
		_page_allocator_create_page(p_mngr, p_allocator->pageSize, page_id, &page);
		p_allocator->activePageId = page_id;
	}

	auto _page_allocator_try_allocate(MeshManager *p_mngr, Page *p_page, uint64 p_size, uint64 p_alignment, _PageAllocation *p_out_allocation) -> bool
	{
		p_out_allocation->allocation = virtual_block_allocate(p_mngr->desc.allocCtx, p_page->block, p_size, p_alignment);
		if (p_out_allocation->allocation)
		{
			p_out_allocation->buffer   = p_page->buffer;
			p_out_allocation->offset   = virtual_allocation_get_offset(p_mngr->desc.allocCtx, p_out_allocation->allocation);
			p_out_allocation->pageId   = p_page->id;
			p_out_allocation->heapSlot = p_page->heapSlot;

			TST_SCOPED_ATOMIC(p_page->activeAllocations, active_allocations);
			++active_allocations;

			return true;
		}
		return false;
	}

	auto _page_allocator_destroy_page(MeshManager *p_mngr, PageAllocator *p_allocator, Page *p_page) -> void
	{
		virtual_block_destroy(p_mngr->desc.allocCtx, p_page->block);
		buffer_destroy(p_mngr->desc.gpuCtx, p_page->buffer);
		gpu::resource_descriptor_heap_free_buffer_slot(p_mngr->desc.gpuCtx, p_mngr->desc.resourceHeap, p_page->heapSlot);
		p_allocator->pageIdAllocator.free_slot(p_page->id);
	}

	auto page_allocator_create(MeshManager *p_mngr, PageAllocator *p_out_allocator, uint64 p_page_size) -> void
	{
		p_out_allocator->pageIdAllocator.capacity = 64u;
		p_out_allocator->pageSize                 = p_page_size;
		_page_allocator_insert_page(p_mngr, p_out_allocator); // Insert the default page
	}

	auto page_allocator_destroy(MeshManager *p_mngr, PageAllocator *p_allocator) -> void
	{
		for (auto &page: p_allocator->pages)
			_page_allocator_destroy_page(p_mngr, p_allocator, &page);
		p_allocator->pages.clear();

		p_allocator->pageSize        = 0u;
		p_allocator->pageIdAllocator = {};
		p_allocator->activePageId    = 0u;
	}

	auto page_allocator_free_page_allocation(MeshManager *p_mngr, PageAllocator *p_allocator, _PageAllocation *p_allocation) -> void
	{
		auto &page{p_allocator->pages[p_allocation->pageId]};

		virtual_block_free(p_mngr->desc.allocCtx, p_allocation->allocation);

		{
			TST_SCOPED_ATOMIC(page.activeAllocations, active_allocations);

			if (active_allocations.fetch_sub(1u) == 0u && p_allocation->pageId != p_allocator->activePageId)
				_page_allocator_destroy_page(p_mngr, p_allocator, &page);
		}

		p_allocation->buffer     = nullptr;
		p_allocation->allocation = nullptr;
		p_allocation->offset     = 0u;
		p_allocation->pageId     = 0u;
		p_allocation->heapSlot   = UINT32_MAX;
	}

	auto page_allocator_allocate_across_pages(MeshManager *    p_mngr, PageAllocator *p_allocator, uint64 p_size, uint64 p_alignment,
											  _PageAllocation *p_out_allocation) -> void
	{
		if (_page_allocator_try_allocate(p_mngr, &p_allocator->pages[p_allocator->activePageId], p_size, p_alignment, p_out_allocation))
			return;

		for (auto &page: p_allocator->pages)
		{
			if (_page_allocator_try_allocate(p_mngr, &page, p_size, p_alignment, p_out_allocation))
				return;
		}

		if (p_size > p_allocator->pageSize) // Create a dedicated allocation
		{
			const uint32 page_id{p_allocator->pageIdAllocator.alloc_slot()};

			Page &page{p_allocator->pages.emplace_back()};
			_page_allocator_create_page(p_mngr, p_size, page_id, &page);
			if (!_page_allocator_try_allocate(p_mngr, &p_allocator->pages[page_id], p_size, p_alignment, p_out_allocation))
				TST_PERMA_ASSERT(false);
			return;
		}

		_page_allocator_insert_page(p_mngr, p_allocator);
		if (!_page_allocator_try_allocate(p_mngr, &p_allocator->pages[p_allocator->activePageId], p_size, p_alignment, p_out_allocation))
		{
			TST_PERMA_ASSERT(false);
		}
	}

	// Returns the total size of all allocated pages
	auto page_allocator_get_total_allocated_size(const PageAllocator *p_allocator) -> uint64
	{
		uint64 total_size{0ull};

		for (const auto &page: p_allocator->pages)
			total_size += page.size;

		return total_size;
	}

	auto mesh_mngr_create(const MeshManagerDesc &p_desc) -> MeshManager *
	{
		const auto mngr{new MeshManager{.desc = p_desc}};

		mtx_init(&mngr->pageMtx, mtx_plain);

		page_allocator_create(mngr, &mngr->vertexPager, MeshManager::vertexPageSize);
		page_allocator_create(mngr, &mngr->meshletPager, MeshManager::meshletPageSize);
		page_allocator_create(mngr, &mngr->meshletVertexPager, MeshManager::meshletVertexPageSize);
		page_allocator_create(mngr, &mngr->meshletTrianglePager, MeshManager::meshletTrianglePageSize);

		gpu::BufferDesc material_indirection_buffer_desc{};
		material_indirection_buffer_desc.size       = MeshManager::materialIndirectionBufferSize * sizeof(uint32);
		material_indirection_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eTransferDst | gpu::EBufferUsageFlagBits::eStorageBuffer;
		material_indirection_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;
		mngr->materialIndirectionBuffer             = gpu::buffer_create(p_desc.gpuCtx, material_indirection_buffer_desc);
		mngr->materialIndirectionBlock              = gpu::virtual_block_create(p_desc.allocCtx, MeshManager::materialIndirectionBufferSize * sizeof(uint32));

		gpu::BufferDesc static_mesh_metadata_buffer_desc{};
		static_mesh_metadata_buffer_desc.size       = 1028u * 1028u * sizeof(StaticMeshMetadata); // 28 Mib / ~ 1 million meshes
		static_mesh_metadata_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eTransferDst | gpu::EBufferUsageFlagBits::eStorageBuffer;
		static_mesh_metadata_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal; // More efficient?
		mngr->staticMeshMetadataBuffer              = gpu::buffer_create(p_desc.gpuCtx, static_mesh_metadata_buffer_desc);

		mngr->staticMeshes.setDestructorUserData(mngr);
		mngr->staticMeshes.setDestructorFn(+[](StaticMesh *p_data, void *p_user_data) -> void
		{
			const auto mngr{static_cast<MeshManager *>(p_user_data)};

			p_data->meshlets.clear();

			// TODO: Freeing these allocations should be deferred (while under the mutex lock in the local deferred scope)
			
			mtx_lock(&mngr->pageMtx);

			// If you just used static_mesh_register, you won't have allocated anything.
			if (p_data->vertexBufferAllocation.allocation)
				page_allocator_free_page_allocation(mngr, &mngr->vertexPager, &p_data->vertexBufferAllocation);
			if (p_data->meshletBufferAllocation.allocation)
				page_allocator_free_page_allocation(mngr, &mngr->meshletPager, &p_data->meshletBufferAllocation);
			if (p_data->meshletVertexBufferAllocation.allocation)
				page_allocator_free_page_allocation(mngr, &mngr->meshletVertexPager, &p_data->meshletVertexBufferAllocation);
			if (p_data->meshletTriangleBufferAllocation.allocation)
				page_allocator_free_page_allocation(mngr, &mngr->meshletTrianglePager, &p_data->meshletTriangleBufferAllocation);
			if (p_data->materialIndirectionBufferAllocation)
				virtual_block_free(mngr->desc.allocCtx, p_data->materialIndirectionBufferAllocation);

			p_data->materialIndirectionBufferAllocation = nullptr;
			p_data->materialIndirectionBufferOffset     = 0u;

			mtx_unlock(&mngr->pageMtx);
		});

		return mngr;
	}

	auto mesh_mngr_destroy(MeshManager *p_mngr) -> void
	{
		gpu::GPUContext *       gpu_ctx{p_mngr->desc.gpuCtx};
		gpu::AllocationContext *alloc_ctx{p_mngr->desc.allocCtx};
		gpu::UploadContext *    upload_ctx{p_mngr->desc.uploadCtx};

		gpu::wait_idle(gpu_ctx); // Make sure all the state trackers are ready to be destroyed
		gpu::poll_uploads(upload_ctx);

		p_mngr->staticMeshes.clear();

		gpu::buffer_destroy(gpu_ctx, p_mngr->staticMeshMetadataBuffer);
		gpu::buffer_destroy(gpu_ctx, p_mngr->materialIndirectionBuffer);
		gpu::virtual_block_destroy(alloc_ctx, p_mngr->materialIndirectionBlock);

		page_allocator_destroy(p_mngr, &p_mngr->vertexPager);
		page_allocator_destroy(p_mngr, &p_mngr->meshletPager);
		page_allocator_destroy(p_mngr, &p_mngr->meshletVertexPager);
		page_allocator_destroy(p_mngr, &p_mngr->meshletTrianglePager);

		mtx_destroy(&p_mngr->pageMtx);

		delete p_mngr;
	}

	auto mesh_mngr_get_static_mesh_metadata_buffer_address(const MeshManager *p_mngr) -> gpu::DeviceAddress
	{
		return gpu::buffer_get_address(p_mngr->desc.gpuCtx, p_mngr->staticMeshMetadataBuffer);
	}

	auto mesh_mngr_get_material_indirection_buffer_address(const MeshManager *p_mngr) -> gpu::DeviceAddress
	{
		return gpu::buffer_get_address(p_mngr->desc.gpuCtx, p_mngr->materialIndirectionBuffer);
	}

	auto static_mesh_register(MeshManager *p_mngr) -> StaticMeshHandle
	{
		return p_mngr->staticMeshes.emplace(StaticMesh{});
	}

	auto static_mesh_upload_data(MeshManager *                 p_mngr, StaticMeshHandle p_static_mesh, gpu::StateTrackerHandle p_state_tracker,
								 const StaticMeshGeometryData &p_geometry_data) -> void
	{
		gpu::AllocationContext *alloc_ctx{p_mngr->desc.allocCtx};
		gpu::UploadContext *    upload_ctx{p_mngr->desc.uploadCtx};

		StaticMesh &static_mesh{p_mngr->staticMeshes[p_static_mesh]};

		static_mesh.meshlets.assign_range(p_geometry_data.meshlets);
		static_mesh.materials.assign_range(p_geometry_data.materials);

		static_mesh.boundingSphere = p_geometry_data.boundingSphere;

		const uint64 vertex_buffer_size{p_geometry_data.vertices.size_bytes()};
		const uint64 meshlet_buffer_size{p_geometry_data.meshlets.size_bytes()};
		const uint64 meshlet_vertex_buffer_size{p_geometry_data.meshletVertices.size_bytes()};
		const uint64 meshlet_triangle_buffer_size{p_geometry_data.meshletIndices.size_bytes()};
		const uint64 material_indirection_buffer_size{p_geometry_data.materials.size_bytes()};

		mtx_lock(&p_mngr->pageMtx);

		page_allocator_allocate_across_pages(p_mngr, &p_mngr->vertexPager, vertex_buffer_size, alignof(StaticMeshVertex), &static_mesh.vertexBufferAllocation);
		page_allocator_allocate_across_pages(p_mngr, &p_mngr->meshletPager, meshlet_buffer_size, alignof(Meshlet), &static_mesh.meshletBufferAllocation);
		page_allocator_allocate_across_pages(p_mngr, &p_mngr->meshletVertexPager, meshlet_vertex_buffer_size, alignof(uint32),
											 &static_mesh.meshletVertexBufferAllocation);
		page_allocator_allocate_across_pages(p_mngr, &p_mngr->meshletTrianglePager, meshlet_triangle_buffer_size, alignof(uint8),
											 &static_mesh.meshletTriangleBufferAllocation);

		static_mesh.materialIndirectionBufferAllocation = gpu::virtual_block_allocate(alloc_ctx, p_mngr->materialIndirectionBlock, material_indirection_buffer_size,
																					  alignof(uint32));
		static_mesh.materialIndirectionBufferOffset = gpu::virtual_allocation_get_offset(alloc_ctx, static_mesh.materialIndirectionBufferAllocation);

		mtx_unlock(&p_mngr->pageMtx);

		// The material ids are just the indices into the pool and also the gpu buffer
		const Vector<uint32> material_indirection_buffer_data{
			static_mesh.materials | std::views::transform([](const MaterialHandle p_handle) -> uint32 { return p_handle.getId(); }) | std::ranges::to<Vector<uint32> >()
		};

		Array<gpu::BufferUploadDesc, 6u> upload_descs;

		gpu::BufferUploadDesc &vertex_upload_desc{upload_descs[0u]};
		vertex_upload_desc.size = vertex_buffer_size;
		vertex_upload_desc.data = p_geometry_data.vertices.data();
		gpu::BufferCopyRegion vertex_copy_region{0ull, static_mesh.vertexBufferAllocation.offset, vertex_buffer_size};
		vertex_upload_desc.uploadRegions = vertex_copy_region;

		gpu::BufferUploadDesc &meshlet_upload_desc{upload_descs[1u]};
		meshlet_upload_desc.size = meshlet_buffer_size;
		meshlet_upload_desc.data = p_geometry_data.meshlets.data();
		gpu::BufferCopyRegion meshlet_copy_region{0ull, static_mesh.meshletBufferAllocation.offset, meshlet_buffer_size};
		meshlet_upload_desc.uploadRegions = meshlet_copy_region;

		gpu::BufferUploadDesc &meshlet_vertex_upload_desc{upload_descs[2u]};
		meshlet_vertex_upload_desc.size = meshlet_vertex_buffer_size;
		meshlet_vertex_upload_desc.data = p_geometry_data.meshletVertices.data();
		gpu::BufferCopyRegion meshlet_vertex_copy_region{0ull, static_mesh.meshletVertexBufferAllocation.offset, meshlet_vertex_buffer_size};
		meshlet_vertex_upload_desc.uploadRegions = meshlet_vertex_copy_region;

		gpu::BufferUploadDesc &meshlet_index_upload_desc{upload_descs[3u]};
		meshlet_index_upload_desc.size = meshlet_triangle_buffer_size;
		meshlet_index_upload_desc.data = p_geometry_data.meshletIndices.data();
		gpu::BufferCopyRegion meshlet_index_copy_region{0ull, static_mesh.meshletTriangleBufferAllocation.offset, meshlet_triangle_buffer_size};
		meshlet_index_upload_desc.uploadRegions = meshlet_index_copy_region;

		gpu::BufferUploadDesc &material_indirection_upload_desc{upload_descs[4u]};
		material_indirection_upload_desc.size = material_indirection_buffer_size;
		material_indirection_upload_desc.data = material_indirection_buffer_data.data();
		gpu::BufferCopyRegion material_indirection_copy_region{0ull, static_mesh.materialIndirectionBufferOffset, material_indirection_buffer_size};
		material_indirection_upload_desc.uploadRegions = material_indirection_copy_region;

		StaticMeshMetadata metadata{};
		metadata.boundingSphere = p_geometry_data.boundingSphere;
		metadata.meshletCount   = static_cast<uint32>(p_geometry_data.meshlets.size());

		metadata.vertexBufferOffset              = static_cast<uint32>(static_mesh.vertexBufferAllocation.offset / sizeof(StaticMeshVertex));
		metadata.meshletBufferOffset             = static_cast<uint32>(static_mesh.meshletBufferAllocation.offset / sizeof(Meshlet));
		metadata.meshletVertexBufferOffset       = static_cast<uint32>(static_mesh.meshletVertexBufferAllocation.offset / sizeof(uint32));
		metadata.meshletTriangleBufferOffset     = static_cast<uint32>(static_mesh.meshletTriangleBufferAllocation.offset / sizeof(uint8));
		metadata.materialIndirectionBufferOffset = static_cast<uint32>(static_mesh.materialIndirectionBufferOffset / sizeof(uint32));

		metadata.vertexBufferPageSlot          = static_cast<uint8>(static_mesh.vertexBufferAllocation.heapSlot);
		metadata.meshletBufferPageSlot         = static_cast<uint8>(static_mesh.meshletBufferAllocation.heapSlot);
		metadata.meshletVertexBufferPageSlot   = static_cast<uint8>(static_mesh.meshletVertexBufferAllocation.heapSlot);
		metadata.meshletTriangleBufferPageSlot = static_cast<uint8>(static_mesh.meshletTriangleBufferAllocation.heapSlot);

		const uint64 metadata_offset{p_static_mesh.getId() * sizeof(StaticMeshMetadata)};

		gpu::BufferUploadDesc &metadata_upload_desc{upload_descs[5u]};
		metadata_upload_desc.size = sizeof(StaticMeshMetadata);
		metadata_upload_desc.data = &metadata;
		gpu::BufferCopyRegion metadata_copy_region{0ull, metadata_offset, sizeof(StaticMeshMetadata)};
		metadata_upload_desc.uploadRegions = metadata_copy_region;

		gpu::upload_data_to_buffers(upload_ctx, {
										static_mesh.vertexBufferAllocation.buffer,
										static_mesh.meshletBufferAllocation.buffer,
										static_mesh.meshletVertexBufferAllocation.buffer,
										static_mesh.meshletTriangleBufferAllocation.buffer,
										p_mngr->materialIndirectionBuffer,
										p_mngr->staticMeshMetadataBuffer
									}, upload_descs, p_state_tracker);
	}

	auto static_mesh_create(MeshManager *p_mngr, gpu::StateTrackerHandle p_state_tracker, const StaticMeshGeometryData &p_geometry_data) -> StaticMeshHandle
	{
		const StaticMeshHandle out_handle{static_mesh_register(p_mngr)};
		static_mesh_upload_data(p_mngr, out_handle, p_state_tracker, p_geometry_data);
		return out_handle;
	}

	auto static_mesh_destroy(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> void
	{
		p_mngr->staticMeshes.destroy(p_static_mesh);
	}

	auto static_mesh_get(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> const StaticMesh &
	{
		return p_mngr->staticMeshes[p_static_mesh];
	}

	auto static_mesh_get(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> StaticMesh &
	{
		return p_mngr->staticMeshes[p_static_mesh];
	}

	auto static_mesh_try_get(const MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> const StaticMesh *
	{
		return p_mngr->staticMeshes.try_get(p_static_mesh);
	}

	auto static_mesh_try_get(MeshManager *p_mngr, StaticMeshHandle p_static_mesh) -> StaticMesh *
	{
		return p_mngr->staticMeshes.try_get(p_static_mesh);
	}
}
