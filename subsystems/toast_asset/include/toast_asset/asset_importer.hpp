#pragma once

#include <filesystem>

#include "asset.hpp"

namespace toaster::asset
{
	class TST_ASSET_API IAssetImporter
	{
	public:
		virtual ~IAssetImporter() = default;

		virtual auto loadAssetFromFile(const std::filesystem::path &p_path) -> RefPtr<IAsset> = 0;
	};
}
