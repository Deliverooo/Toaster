#include "toast_render/transform_system.hpp"
#include "toast_render/render_context.hpp"

namespace toaster::rnd
{
	TransformSystem::TransformSystem(RenderContext *p_render_ctx, uint32 p_max_transforms, uint32 p_frames_in_flight) : m_renderCtx(p_render_ctx),
																														m_maxTransforms(p_max_transforms)
	{
		m_freeTransformList = FreelistAllocator<uint32>{m_maxTransforms};

		gpu::BufferDesc transform_buffer_desc{};
		transform_buffer_desc.size       = m_maxTransforms * sizeof(math::Float4x4);
		transform_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		transform_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		m_transformSSBOs.resize(p_frames_in_flight);
		for (auto &ssbo: m_transformSSBOs)
			ssbo = gpu::buffer_create(m_renderCtx->gpuCtx, transform_buffer_desc);
	}

	TransformSystem::~TransformSystem()
	{
		for (auto ssbo: m_transformSSBOs)
			gpu::buffer_destroy(m_renderCtx->gpuCtx, ssbo);
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

	auto TransformSystem::updateTransform(uint32 p_transform_id, uint32 p_frame_index, math::Matrix p_transform) -> void
	{
		TST_ASSERT(p_transform_id < m_maxTransforms);
		void *          mapped{gpu::buffer_get_mapped_data(m_renderCtx->gpuCtx, m_transformSSBOs[p_frame_index])};
		math::Float4x4 *mapped_transform{
			static_cast<math::Float4x4 *>(reinterpret_cast<void *>(reinterpret_cast<uint64>(mapped) + p_transform_id * sizeof(math::Float4x4)))
		};
		XMStoreFloat4x4(mapped_transform, p_transform);
	}
}
