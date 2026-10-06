#pragma once

#include <filesystem>

#include "toast_render/texture.hpp"
#include "toast_asset.hpp"

#include <taskflow/taskflow.hpp>

#include "toast_lib/buffer.hpp"

namespace toaster::asset
{
	auto TST_ASSET_API loadTextureIntoBuffer(StringView p_path, gpu::EFormat &p_out_format, uint32 &p_out_width, uint32 &p_out_height) -> DataBuffer;

	auto TST_ASSET_API asyncLoadTextureFromFile(render::TextureHandle p_dst_texture, render::TextureManager &p_texture_manager, const std::filesystem::path &p_path,
												tf::Executor &        p_executor) -> void;
}
