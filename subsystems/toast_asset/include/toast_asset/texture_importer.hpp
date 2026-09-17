#pragma once

#include <filesystem>
#include <queue>

#include "toast_render/texture.hpp"
#include "toast_asset.hpp"

namespace toaster::asset
{
	class TST_ASSET_API TextureImporter
	{
	public:
		TextureImporter() = default;
		~TextureImporter();

		auto asyncLoadTextureFromFile(render::TextureManager *p_texture_manager, render::TextureHandle p_dst_texture, const std::filesystem::path &p_path) -> void;

		auto waitImports() -> void;

	private:
		struct ImportTask
		{
			String                path;
			render::TextureHandle dstTexture{nullptr};
		};

		std::atomic_bool m_terminationRequested{false};

		std::vector<std::thread> m_pendingImports;
	};
}
