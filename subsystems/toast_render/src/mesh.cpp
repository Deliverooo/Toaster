#include "toast_render/mesh.hpp"

#include <shared_mutex>

#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	MeshManager::MeshManager(RenderContext *p_render_ctx) : m_renderCtx(p_render_ctx)
	{
		m_vertexPager = makeUnique<gpu::alloc::GPUPageAllocator>(pageSize, m_renderCtx->getResourceHeap());
		m_indexPager  = makeUnique<gpu::alloc::GPUPageAllocator>(pageSize, m_renderCtx->getResourceHeap());

		m_staticMeshes.setDestructorUserData(this);
		m_staticMeshes.setDestructorFn(+[](StaticMesh *p_data, void *p_user_data) -> void
		{
			auto ts{static_cast<MeshManager *>(p_user_data)};

			p_data->submeshes.clear();

			ts->m_vertexPager->freePageAllocation(p_data->vertexBufferAllocation);
			ts->m_vertexPager->freePageAllocation(p_data->indexBufferAllocation);

			p_data->vertexBufferAllocation = {};
			p_data->indexBufferAllocation  = {};

			gpu::upload::destroyStateTracker(p_data->stateTracker);
		});
	}

	MeshManager::~MeshManager()
	{
		m_staticMeshes.clear();

		m_vertexPager.reset();
		m_indexPager.reset();
	}

	auto MeshManager::registerStaticMesh() -> StaticMeshHandle
	{
		StaticMesh temp_mesh{};

		// No. Materials do not count as 'subresources'
		temp_mesh.stateTracker = gpu::upload::createStateTracker(2u); // Vertex and index buffer.

		return m_staticMeshes.emplace(std::move(temp_mesh));
	}

	auto MeshManager::uploadStaticMeshData(StaticMeshHandle            p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
										   const std::vector<Submesh> &p_submeshes) -> void
	{
		StaticMesh &static_mesh{m_staticMeshes[p_handle]};
		static_mesh.submeshes = p_submeshes;

		const uint64 vertex_buffer_size{p_vertices.size() * sizeof(StaticMeshVertex)};
		const uint64 index_buffer_size{p_indices.size() * sizeof(uint32)};

		m_vertexPager->allocateAcrossPages(vertex_buffer_size, alignof(StaticMeshVertex), static_mesh.vertexBufferAllocation);
		m_indexPager->allocateAcrossPages(index_buffer_size, alignof(uint32), static_mesh.indexBufferAllocation);

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.vertexBufferAllocation.buffer,
											p_vertices.data(),
											vertex_buffer_size,
											static_mesh.vertexBufferAllocation.offset
										}, static_mesh.stateTracker);
		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											static_mesh.indexBufferAllocation.buffer,
											p_indices.data(),
											index_buffer_size,
											static_mesh.indexBufferAllocation.offset
										}, static_mesh.stateTracker);
	}

	auto MeshManager::createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
									   const std::vector<Submesh> &         p_submeshes) -> StaticMeshHandle
	{
		StaticMesh temp_mesh{};
		temp_mesh.submeshes = p_submeshes;

		const uint64 vertex_buffer_size{p_vertices.size() * sizeof(StaticMeshVertex)};
		const uint64 index_buffer_size{p_indices.size() * sizeof(uint32)};

		m_vertexPager->allocateAcrossPages(vertex_buffer_size, alignof(StaticMeshVertex), temp_mesh.vertexBufferAllocation);
		m_indexPager->allocateAcrossPages(index_buffer_size, alignof(uint32), temp_mesh.indexBufferAllocation);

		// No. Materials do not count as 'subresources'
		temp_mesh.stateTracker = gpu::upload::createStateTracker(2u); // Vertex and index buffer.

		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											temp_mesh.vertexBufferAllocation.buffer,
											p_vertices.data(),
											vertex_buffer_size,
											temp_mesh.vertexBufferAllocation.offset
										}, temp_mesh.stateTracker);
		gpu::upload::uploadDataToBuffer(gpu::upload::BufferUploadDesc{
											temp_mesh.indexBufferAllocation.buffer,
											p_indices.data(),
											index_buffer_size,
											temp_mesh.indexBufferAllocation.offset
										}, temp_mesh.stateTracker);

		return m_staticMeshes.emplace(std::move(temp_mesh));
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
