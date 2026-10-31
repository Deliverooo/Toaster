#pragma once

#include "toast_asset.hpp"
#include "toast_gpu/api.hpp"
#include "toast_lib/buffer.hpp"

#include <filesystem>

namespace toaster::rnd
{
	struct RenderContext;
}

namespace toaster::ast
{
	auto TST_ASSET_API texture_load_into_buffer(StringView p_path, gpu::EFormat &p_out_format, uint32 &p_out_width, uint32 &p_out_height) -> DataBuffer;

	// dst texture, heap slot and state tracker will have to be pre-registered prior to calling this function.
	// See gpu::resource_descriptor_heap_alloc_texture_slot, gpu::texture_register_null and gpu::state_tracker_register
	auto TST_ASSET_API texture_load_from_file(const rnd::RenderContext *    p_render_ctx, gpu::TextureHandle                  p_dst_texture, uint32 p_dst_heap_slot,
											  gpu::StateTrackerHandle p_dst_state_tracker, const std::filesystem::path &p_path,
											  gpu::EFormat            p_desired_format = gpu::EFormat::eR8G8B8A8Srgb) -> void;
}
