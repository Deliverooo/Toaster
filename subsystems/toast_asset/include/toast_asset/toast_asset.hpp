#pragma once

#define TST_ASSET_API // Technically just for consistency across the API

#include <toast_lib/core_basic.hpp>

namespace toaster::asset
{
	struct TST_ASSET_API URI
	{
		String scheme;
		String path;
		String fragment;

		static auto parse(std::string_view p_uri) -> URI
		{
			URI uri;

			uint64 scheme_end{p_uri.find("://")};
			if (scheme_end == std::string_view::npos)
			{
				uri.scheme = "file";
				uri.path   = p_uri;
				return uri;
			}

			uri.scheme = p_uri.substr(0u, scheme_end);
			std::string_view rest{p_uri.substr(scheme_end + 3u)};
			uint64           frag_start{rest.find('#')};
			if (frag_start != std::string_view::npos)
			{
				uri.path     = rest.substr(0u, frag_start);
				uri.fragment = rest.substr(frag_start + 1u);
			}
			else
				uri.path = rest;
			return uri;
		}
	};
}
