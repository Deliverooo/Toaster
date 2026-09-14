#pragma once

#include <filesystem>

#include "toast_render/texture.hpp"
#include "toast_asset.hpp"

namespace toaster::asset
{
	class TST_ASSET_API TextureImporter
	{
	public:
		TextureImporter(render::TextureManager *p_texture_manager);

		auto importFromFile(const std::filesystem::path &p_path) -> render::TextureHandle;

	private:
		NonOwningPtr<render::TextureManager> m_textureManager{nullptr};
	};
}
