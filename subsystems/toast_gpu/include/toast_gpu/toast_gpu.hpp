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
#include "toast_lib/handle.hpp"

namespace toaster::gpu
{
	#define TST_DECLARE_GPU_HANDLE(__type) struct __type;\
		using __type##Handle = Handle<__type>
}
