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
		TextureImporter(render::TextureManager *p_texture_manager);
		~TextureImporter();

		auto asyncLoadTextureFromFile(render::TextureHandle p_dst_texture, const std::filesystem::path &p_path) -> void;

		auto waitImports() -> void;

		auto getTextureManager() const -> render::TextureManager * { return m_textureManager; }

	private:
		NonOwningPtr<render::TextureManager> m_textureManager{nullptr};

		struct ImportTask
		{
			String                path;
			render::TextureHandle dstTexture{nullptr};
		};

		std::atomic_bool m_terminationRequested{false};

		std::vector<std::thread> m_pendingImports;

		std::mutex m_mutex;
	};
}
