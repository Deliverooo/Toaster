#pragma once

#include <filesystem>

#include "toast_render/texture.hpp"
#include "toast_asset.hpp"

#include <taskflow/taskflow.hpp>

#include "toast_lib/buffer.hpp"

namespace toaster::ast
{
	auto TST_ASSET_API loadTextureIntoBuffer(StringView p_path, gpu::EFormat &p_out_format, uint32 &p_out_width, uint32 &p_out_height) -> DataBuffer;

	auto TST_ASSET_API asyncLoadTextureFromFile(rnd::TextureHandle p_dst_texture, rnd::TextureManager *p_texture_manager, const std::filesystem::path &p_path,
												tf::Executor &     p_executor, gpu::EFormat            p_desired_format = gpu::EFormat::eR8G8B8A8Srgb) -> void;
	auto TST_ASSET_API asyncLoadTextureFromFile(rnd::TextureHandle p_dst_texture, rnd::TextureManager *p_texture_manager, const std::filesystem::path &p_path,
												tf::Taskflow &     p_taskflow, gpu::EFormat            p_desired_format = gpu::EFormat::eR8G8B8A8Srgb) -> void;
	auto TST_ASSET_API asyncLoadTextureFromFile(rnd::TextureHandle p_dst_texture, rnd::TextureManager *p_texture_manager, const std::filesystem::path &p_path,
												tf::Subflow &      p_subflow, gpu::EFormat             p_desired_format = gpu::EFormat::eR8G8B8A8Srgb) -> void;
}
