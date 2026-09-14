#include "toast_render/mesh.hpp"

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

			gpu::alloc::virtualFree(p_data->indexBufferAllocation);
			gpu::alloc::virtualFree(p_data->vertexBufferAllocation);
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

	auto MeshManager::createStaticMesh(const std::vector<StaticMeshVertex> &p_vertices, const std::vector<uint32> &p_indices,
									   const std::vector<Submesh> &         p_submeshes) -> StaticMeshHandle
	{
		const uint64 vertex_buffer_size{p_vertices.size() * sizeof(StaticMeshVertex)};
		const uint64 index_buffer_size{p_indices.size() * sizeof(uint32)};

		StaticMesh static_mesh{};
		static_mesh.submeshes              = p_submeshes;
		static_mesh.vertexBufferAllocation = gpu::alloc::virtualAllocate(m_staticMeshVertexBufferBlock, vertex_buffer_size, 16u);
		static_mesh.indexBufferAllocation  = gpu::alloc::virtualAllocate(m_staticMeshIndexBufferBlock, index_buffer_size, 16u);

		const uint64 vertex_offset{gpu::alloc::getAllocationOffset(static_mesh.vertexBufferAllocation)};
		const uint64 index_offset{gpu::alloc::getAllocationOffset(static_mesh.indexBufferAllocation)};

		gpu::upload::uploadDataToBuffer(m_staticMeshVertexBuffer, p_vertices.data(), vertex_buffer_size, vertex_offset);
		gpu::upload::uploadDataToBuffer(m_staticMeshIndexBuffer, p_indices.data(), index_buffer_size, index_offset);

		return m_staticMeshes.emplace(std::move(static_mesh));
	}

	auto MeshManager::destroyStaticMesh(StaticMeshHandle p_handle) -> void
	{
		m_staticMeshes.destroy(p_handle);
	}
}
