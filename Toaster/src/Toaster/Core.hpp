#pragma once

// check if the platform is Windows
#ifdef TST_PLATFORM_WINDOWS
// Define the TST_API macro for DLL export/import
#ifdef TST_DYNAMIC_LINK
	#ifdef TST_BUILD_DLL
		#define TST_API __declspec(dllexport)
	#else
		#define TST_API __declspec(dllimport)
	#endif
#else
#define TST_API
#endif
#endif

#define BIT(x) (1 << (x))

namespace tst
{
	template<typename T>
	using ScopedPtr = std::unique_ptr<T>;

	template<typename T>
	using RefPtr = std::shared_ptr<T>;

}
