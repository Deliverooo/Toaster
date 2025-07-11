workspace "Toaster"
	architecture "x64"

	configurations{
		"Debug",
		"Release",
		"Dist"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Toaster/thirdparty/GLFW/include"
IncludeDir["glad"] = "Toaster/thirdparty/glad/include"
IncludeDir["imgui"] = "Toaster/thirdparty/imgui/"
include "Toaster/thirdparty/GLFW"
include "Toaster/thirdparty/glad"
include "Toaster/thirdparty/imgui"

project "Toaster"
	location "Toaster"
	kind "SharedLib"
	language "C++"

	characterset ("Unicode")

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	pchheader "tstpch.h"

	pchsource "%{prj.name}/src/tstpch.cpp"

	files{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
	}

	includedirs{
		"%{prj.name}/src",
		"%{prj.name}/thirdparty/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.imgui}",
	}

	links{
		"GLFW",
		"glad",
		"imgui",
		"opengl32.lib",
	}
	
	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines{
			"TST_PLATFORM_WINDOWS",
			"TST_BUILD_DLL",
			"GLFW_INCLUDE_NONE",
		}

		postbuildcommands{
			("{COPYFILE} %{cfg.buildtarget.relpath} ../bin/" .. outputDir .. "/SandBox")
		}

	filter "configurations:Debug"
		defines "TST_DEBUG"
		buildoptions {"/MDd", "/utf-8"}
		symbols "On"

	filter "configurations:Release"
		defines "TST_RELEASE"
		buildoptions {"/MDd", "/utf-8"}
		optimize "On"

	filter "configurations:Dist"
		defines "TST_DIST"
		buildoptions {"/MDd", "/utf-8"}
		optimize "On"

project "SandBox"
	location "SandBox"
	kind "ConsoleApp"
	language "C++"

	characterset ("Unicode")


	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
	}

	includedirs{
		"Toaster/thirdparty/spdlog/include",
		"Toaster/src"
	}

	links{
		"Toaster"
	}
	
	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines{
			"TST_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "TST_DEBUG"
		buildoptions {"/MDd", "/utf-8"}
		symbols "On"

	filter "configurations:Release"
		defines "TST_RELEASE"
		buildoptions {"/MDd", "/utf-8"}
		optimize "On"

	filter "configurations:Dist"
		defines "TST_DIST"
		buildoptions {"/MDd", "/utf-8"}
		optimize "On"