project "glad"
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files
	{
		"include/glad/glad.h",
		"include/glad/khrplatfoem.h",
		"src/glad.c",
	}

	includedirs{
		"include"
	}


	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"

	filter { "system:windows", "configurations:Debug-AS" }	
		runtime "Debug"

	filter "configurations:Release"
		runtime "Release"

    filter "configurations:Dist"
		runtime "Release"

	