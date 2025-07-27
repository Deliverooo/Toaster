workspace "Toaster"
	architecture "x64"

	configurations{
		"Debug",
		"Release",
		"Dist"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "dependencies/GLFW/include"
IncludeDir["Glad"] = "dependencies/Glad/include"
IncludeDir["ImGui"] = "dependencies/ImGui"
IncludeDir["spdlog"] = "dependencies/spdlog/include"
IncludeDir["stbImage"] = "dependencies/stbImage/include"
IncludeDir["glm"] = "dependencies/glm"
IncludeDir["Entt"] = "dependencies/Entt/include"

include "dependencies/GLFW/"
include "dependencies/Glad/"
include "dependencies/ImGui/"
include "dependencies/spdlog/"
include "dependencies/stbImage/"
include "dependencies/glm/"

project "Toaster"
	location "Toaster"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "On"


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
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.stbImage}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Entt}",
		"C:/VulkanSDK/1.4.313.2/Include",
	}

	libdirs{
		"C:/VulkanSDK/1.4.313.2/Lib",
	}

	links{
		"vulkan-1.lib",
		"GLFW",
		"glad",
		"imgui",
		"spdlog",
		"stbImage",
		"glm",
		"opengl32.lib",
	}
	
	filter "system:windows"
		systemversion "latest"

		defines{
			"TST_PLATFORM_WINDOWS",
			"TST_BUILD_DLL",
			"GLFW_INCLUDE_NONE",
		}

	filter "configurations:Debug"
		defines "TST_DEBUG"
		buildoptions {"/utf-8"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "TST_RELEASE"
		buildoptions {"/utf-8"}
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "TST_DIST"
		buildoptions {"/utf-8"}
		runtime "Release"
		optimize "on"

project "SandBox"
	location "SandBox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "On"
	
	characterset ("Unicode")

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
	}

	includedirs{
		"Toaster/src",
		"dependencies/spdlog/include",
		"dependencies/imgui",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Entt}",

	}

	links{
		"Toaster",
	}
	
	filter "system:windows"
		systemversion "latest"

		defines{
			"TST_PLATFORM_WINDOWS"
		}


	filter "configurations:Debug"
		defines "TST_DEBUG"
		buildoptions {"/utf-8"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "TST_RELEASE"
		buildoptions {"/utf-8"}
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "TST_DIST"
		buildoptions {"/utf-8"}
		runtime "Release"
		optimize "on"


project "ToasterEditor"
	location "ToasterEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "On"
	
	characterset ("Unicode")

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
	}

	includedirs{
		"Toaster/src",
		"dependencies/spdlog/include",
		"dependencies/imgui",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Entt}",

	}

	links{
		"Toaster",
	}
	
	filter "system:windows"
		systemversion "latest"

		defines{
			"TST_PLATFORM_WINDOWS"
		}


	filter "configurations:Debug"
		defines "TST_DEBUG"
		buildoptions {"/utf-8"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "TST_RELEASE"
		buildoptions {"/utf-8"}
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "TST_DIST"
		buildoptions {"/utf-8"}
		runtime "Release"
		optimize "on"