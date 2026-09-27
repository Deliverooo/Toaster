#include "toast_render/transform_system.hpp"

namespace toaster::rd
{
	TransformSystem::TransformSystem(uint32 p_max_transforms, uint32 p_frames_in_flight) : m_maxTransforms(p_max_transforms)
	{
		m_freeTransformList = FreelistAllocator<uint32>{m_maxTransforms};

		gpu::BufferDesc transform_buffer_desc{};
		transform_buffer_desc.size       = m_maxTransforms * sizeof(XMFLOAT4X4);
		transform_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		transform_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		m_transformSSBOs.resize(p_frames_in_flight);
		for (auto &ssbo: m_transformSSBOs)
			ssbo = gpu::createBuffer(transform_buffer_desc);
	}

	TransformSystem::~TransformSystem()
	{
		for (auto ssbo: m_transformSSBOs)
			gpu::destroyBuffer(ssbo);
		m_transformSSBOs.clear();
	}

	auto TransformSystem::createTransform() -> uint32
	{
		uint32 slot{m_freeTransformList.allocSlot()};

		for (uint32 i{0u}; i < 3u; ++i)
			updateTransform(slot, i, XMMatrixIdentity());

		return slot;
	}

	auto TransformSystem::destroyTransform(uint32 p_transform_id) -> void
	{
		TST_ASSERT(p_transform_id < m_maxTransforms);
		m_freeTransformList.freeSlot(p_transform_id);
	}

	auto TransformSystem::updateTransform(uint32 p_transform_id, uint32 p_frame_index, XMMATRIX p_transform) -> void
	{
		TST_ASSERT(p_transform_id < m_maxTransforms);
		void *      mapped{gpu::getBufferMappedData(m_transformSSBOs[p_frame_index])};
		XMFLOAT4X4 *mapped_transform{static_cast<XMFLOAT4X4 *>(reinterpret_cast<void *>(reinterpret_cast<uint64>(mapped) + p_transform_id * sizeof(XMFLOAT4X4)))};
		XMStoreFloat4x4(mapped_transform, p_transform);
	}
}
