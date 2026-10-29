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
		Vector<math::Float4x4 *>   transformMapped;
	};

	auto transform_mngr_create(RenderContext *p_render_ctx, uint32 p_max_transforms) -> TransformManager *
	{
		const auto mngr{new TransformManager{}};

		mngr->rndCtx            = p_render_ctx;
		mngr->freeTransformList = FreelistAllocator<uint32>{p_max_transforms};

		gpu::BufferDesc transform_buffer_desc{};
		transform_buffer_desc.size       = p_max_transforms * sizeof(math::Float4x4);
		transform_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		transform_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		mngr->transformSSBOs.resize(p_render_ctx->maxFramesInFlight);
		mngr->transformBDAs.resize(p_render_ctx->maxFramesInFlight);
		mngr->transformMapped.resize(p_render_ctx->maxFramesInFlight);
		for (uint32 i{0u}; i < p_render_ctx->maxFramesInFlight; ++i)
		{
			mngr->transformSSBOs[i]  = gpu::buffer_create(p_render_ctx->gpuCtx, transform_buffer_desc);
			mngr->transformBDAs[i]   = gpu::buffer_get_address(p_render_ctx->gpuCtx, mngr->transformSSBOs[i]);
			mngr->transformMapped[i] = static_cast<math::Float4x4 *>(gpu::buffer_get_mapped_data(p_render_ctx->gpuCtx, mngr->transformSSBOs[i]));
		}

		return mngr;
	}

	auto transform_mngr_destroy(TransformManager *p_mngr) -> void
	{
		for (const auto ssbo: p_mngr->transformSSBOs)
			gpu::buffer_destroy(p_mngr->rndCtx->gpuCtx, ssbo);
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
		const uint32 slot{p_mngr->freeTransformList.allocSlot()};

		for (uint32 i{0u}; i < p_mngr->rndCtx->maxFramesInFlight; ++i)
			transform_update(p_mngr, slot, i, p_matrix);

		return slot;
	}

	auto transform_destroy(TransformManager *p_mngr, TransformID p_transform) -> void
	{
		p_mngr->freeTransformList.freeSlot(p_transform);
	}

	auto TST_MATH_CALLCONV transform_update(TransformManager *p_mngr, TransformID p_transform, uint32 p_frame_index, math::FMatrix p_matrix) -> void
	{
		math::Float4x4 *mapped{&p_mngr->transformMapped[p_frame_index][p_transform]};
		XMStoreFloat4x4(mapped, p_matrix);
	}
}
