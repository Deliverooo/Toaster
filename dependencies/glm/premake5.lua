project "glm"
	kind "StaticLib"
	language "C++"

    staticruntime "on"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

    includedirs {
        "glm"
    }

    -- Add glm source files
    files {
        "glm/**.cpp",
        "glm/**.hpp",
        "glm/**.inl"
    }

    -- Define required macro
    defines {
        "GLM_ENABLE_EXPERIMENTAL",
        "GLM_FORCE_RADIANS"
    }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"