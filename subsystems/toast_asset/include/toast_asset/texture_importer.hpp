#pragma once

#include <filesystem>

#include "toast_render/texture.hpp"
#include "toast_asset.hpp"

#include <taskflow/taskflow.hpp>

namespace toaster::asset
{
	auto TST_ASSET_API asyncLoadTextureFromFile(render::TextureHandle p_dst_texture, render::TextureManager &p_texture_manager, const std::filesystem::path &p_path,
												tf::Executor &        p_executor) -> void;
}
