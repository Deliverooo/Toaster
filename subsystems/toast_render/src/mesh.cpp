#include "toast_render/mesh.hpp"

#include <shared_mutex>

#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	MeshManager::MeshManager(uint64 p_max_static_mesh_vertices, uint64 p_max_static_mesh_indices) : m_maxStaticMeshVertices(p_max_static_mesh_vertices),
																									m_maxStaticMeshIndices(p_max_static_mesh_indices)
	{
		const uint64    vertex_buffer_size{m_maxStaticMeshVertices * sizeof(StaticMeshVertex)};
		gpu::BufferDesc vertex_buffer_desc{};
		vertex_buffer_desc.size       = vertex_buffer_size;
		vertex_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer | gpu::EBufferUsageFlagBits::eTransferDst;
		vertex_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;
		m_staticMeshVertexBuffer      = gpu::createBuffer(vertex_buffer_desc);

		const uint64    index_buffer_size{m_maxStaticMeshIndices * sizeof(uint32)};
		gpu::BufferDesc index_buffer_desc{};
		index_buffer_desc.size       = index_buffer_size;
		index_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer | gpu::EBufferUsageFlagBits::eTransferDst;
		index_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;
		m_staticMeshIndexBuffer      = gpu::createBuffer(index_buffer_desc);

		m_staticMeshVertexBufferBlock = gpu::alloc::createVirtualBlock(vertex_buffer_size);
		m_staticMeshIndexBufferBlock  = gpu::alloc::createVirtualBlock(index_buffer_size);

		m_staticMeshes.setDestructorFn(+[](StaticMesh *p_data, void *) -> void
		{
			p_data->submeshes.clear();

			if (p_data->indexBufferAllocation)
				gpu::alloc::virtualFree(p_data->indexBufferAllocation);
			if (p_data->vertexBufferAllocation)
				gpu::alloc::virtualFree(p_data->vertexBufferAllocation);

			p_data->vertexBufferOffset = 0u;
			p_data->indexBufferOffset  = 0u;

			p_data->state->store(EMeshState::eUnloaded); // The mesh no longer exists, so it is 'unloaded'
		});
	}

	MeshManager::~MeshManager()
	{
		m_staticMeshes.clear();

		gpu::upload::cancelBufferUpload(m_staticMeshIndexBuffer);
		gpu::upload::cancelBufferUpload(m_staticMeshVertexBuffer);

		gpu::frame::defferBufferDeletion(m_staticMeshIndexBuffer);
		gpu::frame::defferBufferDeletion(m_staticMeshVertexBuffer);

		gpu::alloc::destroyVirtualBlock(m_staticMeshIndexBufferBlock);
		gpu::alloc::destroyVirtualBlock(m_staticMeshVertexBufferBlock);
	}

	auto MeshManager::registerStaticMesh() -> StaticMeshHandle
	{
		StaticMesh temp_mesh{};
		temp_mesh.state = makeUnique<std::atomic<EMeshState> >(EMeshState::eUnloaded);

		return m_staticMeshes.emplace(std::move(temp_mesh));
	}

	auto MeshManager::uploadStaticMeshData(StaticMeshHandle            p_handle, const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
										   const std::vector<Submesh> &p_submeshes) -> void
	{
		const uint64 vertex_buffer_size{p_vertices.size() * sizeof(StaticMeshVertex)};
		const uint64 index_buffer_size{p_indices.size() * sizeof(uint32)};

		StaticMesh &static_mesh{m_staticMeshes[p_handle]};

		static_mesh.submeshes = p_submeshes;

		static_mesh.vertexBufferAllocation = gpu::alloc::virtualAllocate(m_staticMeshVertexBufferBlock, vertex_buffer_size, alignof(StaticMeshVertex));
		static_mesh.indexBufferAllocation  = gpu::alloc::virtualAllocate(m_staticMeshIndexBufferBlock, index_buffer_size, alignof(uint32));

		const uint64 vertex_allocation_offset{gpu::alloc::getAllocationOffset(static_mesh.vertexBufferAllocation)};
		const uint64 index_allocation_offset{gpu::alloc::getAllocationOffset(static_mesh.indexBufferAllocation)};

		static_mesh.vertexBufferOffset = vertex_allocation_offset / sizeof(StaticMeshVertex);
		static_mesh.indexBufferOffset  = index_allocation_offset / sizeof(uint32);

		static_mesh.vertexReadyToken = gpu::upload::uploadDataToBuffer(m_staticMeshVertexBuffer, p_vertices.data(), vertex_buffer_size, vertex_allocation_offset);
		static_mesh.indexReadyToken  = gpu::upload::uploadDataToBuffer(m_staticMeshIndexBuffer, p_indices.data(), index_buffer_size, index_allocation_offset);

		static_mesh.state->store(EMeshState::eUploadingToGPU);

		{
			std::scoped_lock<std::mutex> lock{m_meshStateMutex};
			m_pendingMeshUploads.insert(p_handle);
		}
	}

	auto MeshManager::createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
									   const std::vector<Submesh> &         p_submeshes) -> StaticMeshHandle
	{
		const uint64 vertex_buffer_size{p_vertices.size() * sizeof(StaticMeshVertex)};
		const uint64 index_buffer_size{p_indices.size() * sizeof(uint32)};

		StaticMesh temp_mesh{};

		temp_mesh.submeshes = p_submeshes;

		temp_mesh.vertexBufferAllocation = gpu::alloc::virtualAllocate(m_staticMeshVertexBufferBlock, vertex_buffer_size, alignof(StaticMeshVertex));
		temp_mesh.indexBufferAllocation  = gpu::alloc::virtualAllocate(m_staticMeshIndexBufferBlock, index_buffer_size, alignof(uint32));

		const uint64 vertex_allocation_offset{gpu::alloc::getAllocationOffset(temp_mesh.vertexBufferAllocation)};
		const uint64 index_allocation_offset{gpu::alloc::getAllocationOffset(temp_mesh.indexBufferAllocation)};

		temp_mesh.vertexBufferOffset = vertex_allocation_offset / sizeof(StaticMeshVertex);
		temp_mesh.indexBufferOffset  = index_allocation_offset / sizeof(uint32);

		temp_mesh.vertexReadyToken = gpu::upload::uploadDataToBuffer(m_staticMeshVertexBuffer, p_vertices.data(), vertex_buffer_size, vertex_allocation_offset);
		temp_mesh.indexReadyToken  = gpu::upload::uploadDataToBuffer(m_staticMeshIndexBuffer, p_indices.data(), index_buffer_size, index_allocation_offset);

		temp_mesh.state = makeUnique<std::atomic<EMeshState> >(EMeshState::eUploadingToGPU);

		return m_staticMeshes.emplace(std::move(temp_mesh));
	}

	auto MeshManager::destroyStaticMesh(StaticMeshHandle p_handle) -> void
	{
		{
			std::scoped_lock<std::mutex> lock{m_meshStateMutex};

			if (m_pendingMeshUploads.contains(p_handle))
				m_pendingMeshUploads.erase(p_handle);
		}

		m_staticMeshes.destroy(p_handle);
	}

	auto MeshManager::pollMeshUploads() -> void
	{
		std::scoped_lock<std::mutex> lock{m_meshStateMutex};

		if (m_pendingMeshUploads.empty())
			return;

		uint64 transfer_value{gpu::getSemaphoreValue(gpu::frame::getTransferTimelineSemaphore())};
		for (auto it{m_pendingMeshUploads.begin()}; it != m_pendingMeshUploads.end();)
		{
			StaticMesh &static_mesh{m_staticMeshes[*it]};
			if (transfer_value >= static_mesh.vertexReadyToken && transfer_value >= static_mesh.indexReadyToken)
			{
				static_mesh.state->store(EMeshState::eReady);
				it = m_pendingMeshUploads.erase(it);
			}
			else
				++it;
		}
	}
}
