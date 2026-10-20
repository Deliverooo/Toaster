#pragma once

#ifdef TST_LIB_BUILD_DLL
#ifdef TST_LIB_DLL_EXPORT
#define TST_LIB_API __declspec(dllexport)
#else
#define TST_LIB_API __declspec(dllimport)
#endif
#else
#define TST_LIB_API
#endif

#include <stdbool.h>