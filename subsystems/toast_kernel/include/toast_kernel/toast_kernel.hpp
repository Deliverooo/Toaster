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

#include "toast_lib/enum.hpp"

namespace toaster::knl
{
	TST_DECLARE_FLAGS_FOR_NAMESPACE();
}
