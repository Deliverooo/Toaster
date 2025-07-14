project "spdlog"
	kind "StaticLib"
	language "C++"

    staticruntime "on"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

    -- Include spdlog headers
    includedirs {
        "include"
    }

    -- Add spdlog source files
    files {
        "src/**.cpp",
        -- "include/spdlog/**.h"
    }

    -- Define required macro
    defines {
        "SPDLOG_COMPILED_LIB"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions {"/utf-8"}
    
    filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"