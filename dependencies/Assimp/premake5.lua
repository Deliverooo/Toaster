project "Assimp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"

    characterset ("Unicode")

    targetdir ("bin/" .. outputDir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

    files{
        "code/**.h",
        "code/**.c",
        "code/**.cpp",
        "code/**.hpp",
        "code/**.inl",

        "include/assimp/**.h",
        "include/assimp/**.hpp",
        "include/assimp/**.inl",
    }

    defines {
        "ASSIMP_BUILD_NO_OWN_ZLIB",
        "ASSIMP_BUILD_NO_OWN",
        "ASSIMP_BUILD_NO_SHARED",
    }

    includedirs{
        "include",
    }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"