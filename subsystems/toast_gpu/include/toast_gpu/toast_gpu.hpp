#pragma once

#ifdef TST_GPU_BUILD_DLL
#ifdef TST_GPU_DLL_EXPORT
#define TST_GPU_API __declspec(dllexport)
#else
#define TST_GPU_API __declspec(dllimport)
#endif
#else
#define TST_GPU_API
#endif

#include "toast_lib/core_basic.hpp"
#include "toast_lib/enum.hpp"

namespace toaster::gpu
{
	TST_DECLARE_FLAGS_FOR_NAMESPACE()
}
