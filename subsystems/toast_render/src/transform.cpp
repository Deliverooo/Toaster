#include "toast_render/transform.hpp"
#include "toast_render/render_context.hpp"

#include "toast_lib/freelist_allocator.hpp"

#include "toast_gpu/api.hpp"

namespace toaster::rnd
{
	struct TransformManager
	{
		RenderContext *            rndCtx{nullptr};
		FreelistAllocator<uint32>  freeTransformList;
		Vector<gpu::BufferHandle>  transformSSBOs;
		Vector<gpu::DeviceAddress> transformBDAs;
		Vector<void *>             transformMapped;
	};

	auto transform_mngr_create(RenderContext *p_render_ctx, uint32 p_max_transforms) -> TransformManager *
	{
		const auto mngr{new TransformManager{}};

		mngr->rndCtx = p_render_ctx;
		freelist_allocator_create(mngr->freeTransformList, p_max_transforms);

		gpu::BufferDesc transform_buffer_desc{};
		transform_buffer_desc.size       = p_max_transforms * sizeof(math::Float4x4);
		transform_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		transform_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		mngr->transformSSBOs.resize(p_render_ctx->maxFramesInFlight);
		mngr->transformBDAs.resize(p_render_ctx->maxFramesInFlight);
		mngr->transformMapped.resize(p_render_ctx->maxFramesInFlight);
		gpu::buffer_create_n(p_render_ctx->gpuCtx, transform_buffer_desc, mngr->transformSSBOs);
		gpu::buffer_get_address_n(p_render_ctx->gpuCtx, mngr->transformSSBOs, mngr->transformBDAs);
		gpu::buffer_get_mapped_data_n(p_render_ctx->gpuCtx, mngr->transformSSBOs, mngr->transformMapped);

		return mngr;
	}

	auto transform_mngr_destroy(TransformManager *p_mngr) -> void
	{
		gpu::buffer_destroy_n(p_mngr->rndCtx->gpuCtx, p_mngr->transformSSBOs);
		p_mngr->transformSSBOs.clear();
		p_mngr->transformBDAs.clear();

		delete p_mngr;
	}

	auto transform_mngr_get_transform_buffer_address(const TransformManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress
	{
		return p_mngr->transformBDAs[p_frame_index];
	}

	auto TST_MATH_CALLCONV transform_create(TransformManager *p_mngr, math::FMatrix p_matrix) -> TransformID
	{
		const uint32 slot{freelist_allocator_alloc_slot(p_mngr->freeTransformList)};

		for (uint32 i{0u}; i < p_mngr->rndCtx->maxFramesInFlight; ++i)
			transform_update(p_mngr, slot, i, p_matrix);

		return slot;
	}

	auto transform_destroy(TransformManager *p_mngr, TransformID p_transform) -> void
	{
		freelist_allocator_free_slot(p_mngr->freeTransformList, p_transform);
	}

	auto TST_MATH_CALLCONV transform_update(TransformManager *p_mngr, uint32 p_frame_index, TransformID p_transform, math::FMatrix p_matrix) -> void
	{
		math::Float4x4 *mapped{&static_cast<math::Float4x4 *>(p_mngr->transformMapped[p_frame_index])[p_transform]};
		XMStoreFloat4x4(mapped, p_matrix);
	}
}
