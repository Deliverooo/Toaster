#pragma once

#include "toast_render.hpp"
#include "toast_gpu/fwd.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::rnd
{
	struct RenderContext;
	struct TransformManager;

	// The transform system doesn't store any metadata to do with the transforms, so it doesn't need a pool.
	using TransformID = uint32;

	auto TST_RENDER_API transform_mngr_create(RenderContext *p_render_ctx, uint32 p_max_transforms) -> TransformManager *;
	auto TST_RENDER_API transform_mngr_destroy(TransformManager *p_mngr) -> void;
	auto TST_RENDER_API transform_mngr_get_transform_buffer_address(const TransformManager *p_mngr, uint32 p_frame_index) -> gpu::DeviceAddress;

	auto TST_RENDER_API TST_MATH_CALLCONV transform_create(TransformManager *p_mngr, math::FMatrix p_matrix = XMMatrixIdentity()) -> TransformID;
	auto TST_RENDER_API                   transform_destroy(TransformManager *p_mngr, TransformID p_transform) -> void;
	auto TST_RENDER_API TST_MATH_CALLCONV transform_update(TransformManager *p_mngr, TransformID p_transform, uint32 p_frame_index, math::FMatrix p_matrix) -> void;
}
