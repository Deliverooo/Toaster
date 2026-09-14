#pragma once

#include "toast_asset.hpp"

namespace toaster::asset
{
	using AssetHandle = uint64;
	constexpr AssetHandle invalidAssetHandle{0u};

	class TST_ASSET_API IAsset
	{
	public:
		virtual ~IAsset() = default;

		static auto  getStaticAssetType() -> CString { return "None"; }
		virtual auto getAssetType() const -> CString = 0;
	};
}
