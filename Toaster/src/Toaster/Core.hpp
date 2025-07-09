#pragma once

// check if the platform is Windows
#ifdef TST_PLATFORM_WINDOWS
// Define the TST_API macro for DLL export/import
	#ifdef TST_BUILD_DLL
		#define TST_API __declspec(dllexport)
	#else
		#define TST_API __declspec(dllimport)
	#endif
#endif

#define BIT(x) (1 << x)