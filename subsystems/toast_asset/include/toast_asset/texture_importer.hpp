#pragma once

#include "toast_asset.hpp"

#include "toast_render/texture.hpp"

#include "toast_gpu/api.hpp"
#include "toast_lib/buffer.hpp"

#include <filesystem>

namespace toaster::ast
{
	auto TST_ASSET_API texture_load_into_buffer(StringView p_path, gpu::EFormat &p_out_format, uint32 &p_out_width, uint32 &p_out_height) -> DataBuffer;

	auto TST_ASSET_API texture_load_from_file(rnd::TextureHandle p_dst_texture, rnd::TextureManager *p_texture_manager, const std::filesystem::path &p_path,
											  gpu::EFormat       p_desired_format = gpu::EFormat::eR8G8B8A8Srgb) -> void;
}
