#pragma once

#ifdef TST_KERNEL_BUILD_DLL
#ifdef TST_KERNEL_DLL_EXPORT
#define TST_KERNEL_API __declspec(dllexport)
#else
#define TST_KERNEL_API __declspec(dllimport)
#endif
#else
#define TST_KERNEL_API
#endif

// Everyone should get the core stuff
#include "toast_lib/core_basic.hpp"
#include "toast_lib/enum_flags.hpp"

namespace toaster::ke
{
	TST_DECLARE_FLAGS_FOR_NAMESPACE();
}
