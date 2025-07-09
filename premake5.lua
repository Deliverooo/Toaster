workspace "Toaster"
	architecture "x64"

	configurations{
		"Debug",
		"Release",
		"Dist"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Toaster"
	location "Toaster"
	kind "SharedLib"
	language "C++"


	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
	}

	includedirs{
		"%{prj.name}/src",
		"%{prj.name}/thirdparty/spdlog/include",
	}
	
	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines{
			"TST_PLATFORM_WINDOWS",
			"TST_BUILD_DLL",
		}

		postbuildcommands{
			("{COPYFILE} %{cfg.buildtarget.relpath} ../bin/" .. outputDir .. "/SandBox")
		}

	filter "configurations:Debug"
		defines "TST_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "TST_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "TST_DIST"
		optimize "On"

project "SandBox"
	location "SandBox"
	kind "ConsoleApp"
	language "C++"

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
		symbols "On"

	filter "configurations:Release"
		defines "TST_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "TST_DIST"
		optimize "On"