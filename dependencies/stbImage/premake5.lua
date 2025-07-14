project "stbImage"
    kind "staticlib"
	language "C"

    staticruntime "on"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")


    includedirs {
        "include"
    }

    -- Add stbImage source files
    files {
        "src/stb_image.c",
        "src/stb_image.h",
    }


    filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"