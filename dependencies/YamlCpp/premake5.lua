project "YamlCpp"
kind "StaticLib"
language "C++"

targetdir ("../../bin/" .. outputDir .. "/%{prj.name}")
objdir ("../../bin-int/" .. outputDir .. "/%{prj.name}")

files {
    "src/**.cpp",
    "include/yaml-cpp/**.h",
    "include/yaml-cpp/**.hpp",
}

defines {
	"YAML_CPP_STATIC_DEFINE",
}
includedirs {
    "include",
    "src",
}

filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"