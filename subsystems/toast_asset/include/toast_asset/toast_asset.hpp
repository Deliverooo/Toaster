#pragma once

#ifdef TST_ASSET_BUILD_DLL
#ifdef TST_ASSET_DLL_EXPORT
#define TST_ASSET_API __declspec(dllexport)
#else
#define TST_ASSET_API __declspec(dllimport)
#endif
#else
#define TST_ASSET_API
#endif

// Everyone should get the core stuff
#include "toast_lib/core_basic.hpp"
#include "toast_lib/enum.hpp"
