#pragma once

#include <memory>

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
#else
#define TST_API
#endif

#define BIT(x) (1 << (x))

namespace tst
{
	template<typename T>
	using ScopedPtr = std::unique_ptr<T>;

	template<typename T>
	using RefPtr = std::shared_ptr<T>;

	template<typename T>
	using CachePtr = std::weak_ptr<T>;


	template<typename T, typename ...Args>
	ScopedPtr<T> make_scoped(Args ...args) { return std::make_unique<T>(std::forward<Args>(args)...); }

	template<typename T, typename ...Args>
	RefPtr<T> make_reference(Args ...args) { return std::make_shared<T>(std::forward<Args>(args)...); }


}


#define TST_REL_PATH "C:/dev/Toaster/SandBox/assets/"
#define TST_FONT_PATH "C:/Windows/Fonts/"


#ifdef NO_THANKS
#define TST_ENABLE_PROFILING
#endif