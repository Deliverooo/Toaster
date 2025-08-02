project "Assimp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "On"

    targetdir ("../../bin/" .. outputDir .. "/%{prj.name}")
    objdir ("../../bin-int/" .. outputDir .. "/%{prj.name}")

    files {
        -- Main Assimp code
        "code/**.cpp",
        "code/**.h",
        "include/assimp/**.h",
        "include/assimp/**.hpp",
        "include/assimp/**.inl",
        
        -- Essential contrib libraries
        "contrib/zlib/*.c",
        "contrib/zlib/*.h",
        "contrib/unzip/*.c",
        "contrib/unzip/*.h",
        "contrib/utf8cpp/source/*.h",
        "contrib/irrXML/**.cpp",
        "contrib/irrXML/**.h",
        -- FIXED: Correct pugixml path
        "contrib/pugixml/src/*.hpp",
        "contrib/pugixml/src/*.cpp",
        "contrib/rapidjson/include/**.h",
        "contrib/poly2tri/poly2tri/**.h",
        "contrib/poly2tri/poly2tri/**.cc",
        "contrib/clipper/*.hpp",
        "contrib/clipper/*.cpp",
        
        -- Generated files
        "revision.h",
    }

    -- Exclude problematic files from CMakeLists.txt patterns
    removefiles {
        "contrib/zlib/example.c",
        "contrib/zlib/minigzip.c",
        "contrib/zlib/gz*.c",
        "code/AssetLib/IFC/IFCReaderGen*",
        "contrib/zlib/contrib/**",
        "code/AssetLib/C4D/**",  -- C4D importer uses non-free SDK
        "code/AssetLib/USD/**",  -- Exclude USD importer to avoid TinyUSDZ dependency
    }

    includedirs {
        "include",
        "code",
        ".",
        "contrib",
        "contrib/zlib",
        "contrib/rapidjson/include",
        "contrib/utf8cpp/source",
        "contrib/unzip",
        "contrib/irrXML",
        "contrib/pugixml/src",
        "contrib/poly2tri",
        "contrib/clipper",
        -- Remove tinyusdz include since we're disabling USD
        -- "contrib/tinyusdz",
        "%{cfg.buildtarget.directory}/../include",
    }

    defines {
        -- Core Assimp configuration from CMakeLists.txt
        "ASSIMP_BUILD_NO_EXPORT",
        
        -- Version information (from PROJECT command in CMakeLists.txt)
        "ASSIMP_VERSION_MAJOR=6",
        "ASSIMP_VERSION_MINOR=0", 
        "ASSIMP_VERSION_PATCH=2",
        
        -- Zlib configuration (matches CMakeLists.txt logic)
        "ASSIMP_BUILD_ZLIB",
        
        -- Disable most importers (keep only essential ones for your project)
        "ASSIMP_BUILD_NO_C4D_IMPORTER",
        "ASSIMP_BUILD_NO_3DS_IMPORTER",
        "ASSIMP_BUILD_NO_AC_IMPORTER", 
        "ASSIMP_BUILD_NO_ASE_IMPORTER",
        "ASSIMP_BUILD_NO_B3D_IMPORTER",
        "ASSIMP_BUILD_NO_BVH_IMPORTER",
        "ASSIMP_BUILD_NO_COB_IMPORTER",
        "ASSIMP_BUILD_NO_CSM_IMPORTER",
        "ASSIMP_BUILD_NO_DXF_IMPORTER",
        "ASSIMP_BUILD_NO_HMP_IMPORTER",
        "ASSIMP_BUILD_NO_IFC_IMPORTER",
        "ASSIMP_BUILD_NO_IRR_IMPORTER",
        "ASSIMP_BUILD_NO_IRRMESH_IMPORTER", 
        "ASSIMP_BUILD_NO_LWO_IMPORTER",
        "ASSIMP_BUILD_NO_LWS_IMPORTER",
        "ASSIMP_BUILD_NO_M3D_IMPORTER",
        "ASSIMP_BUILD_NO_MD2_IMPORTER",
        "ASSIMP_BUILD_NO_MD3_IMPORTER",
        "ASSIMP_BUILD_NO_MD5_IMPORTER",
        "ASSIMP_BUILD_NO_MDC_IMPORTER",
        "ASSIMP_BUILD_NO_MDL_IMPORTER",
        "ASSIMP_BUILD_NO_NFF_IMPORTER",
        "ASSIMP_BUILD_NO_NDO_IMPORTER",
        "ASSIMP_BUILD_NO_OFF_IMPORTER",
        "ASSIMP_BUILD_NO_OGRE_IMPORTER",
        "ASSIMP_BUILD_NO_OPENGEX_IMPORTER",
        "ASSIMP_BUILD_NO_PLY_IMPORTER",
        "ASSIMP_BUILD_NO_MS3D_IMPORTER",
        "ASSIMP_BUILD_NO_Q3D_IMPORTER",
        "ASSIMP_BUILD_NO_Q3BSP_IMPORTER",
        "ASSIMP_BUILD_NO_RAW_IMPORTER",
        "ASSIMP_BUILD_NO_SIB_IMPORTER",
        "ASSIMP_BUILD_NO_SMD_IMPORTER",
        "ASSIMP_BUILD_NO_TERRAGEN_IMPORTER",
        "ASSIMP_BUILD_NO_UNREAL_IMPORTER",
        "ASSIMP_BUILD_NO_X_IMPORTER",
        "ASSIMP_BUILD_NO_X3D_IMPORTER",
        "ASSIMP_BUILD_NO_XGL_IMPORTER",
        "ASSIMP_BUILD_NO_VRML_IMPORTER",
        "ASSIMP_BUILD_NO_USD_IMPORTER",  -- Add this line to disable USD importer
        
        -- Enable only formats you need (comment out to enable)
        -- "ASSIMP_BUILD_NO_FBX_IMPORTER",
        -- "ASSIMP_BUILD_NO_OBJ_IMPORTER", 
        -- "ASSIMP_BUILD_NO_GLTF_IMPORTER",
        -- "ASSIMP_BUILD_NO_BLEND_IMPORTER",  -- Make sure this is commented out
        
        -- Disable M3D and USD (from CMakeLists.txt options)
    }

    -- Windows-specific settings (matches CMakeLists.txt Windows section)
    filter "system:windows"
        systemversion "latest"
        buildoptions { 
            "/utf-8",           -- Source charset (from CMakeLists.txt)
            "/bigobj",          -- Large object files support 
            "/MP",              -- Multi-core compilation (from CMakeLists.txt)
            "/wd4244",          -- Disable double to float warnings
            "/wd4251",          -- Disable DLL interface warnings
            "/wd4351",          -- Array initialization warning (MSVC2013)
        }
        defines {
            "_CRT_SECURE_NO_WARNINGS",     -- From CMakeLists.txt
            "_SCL_SECURE_NO_WARNINGS",     -- From CMakeLists.txt  
            "WIN32_LEAN_AND_MEAN",         -- From CMakeLists.txt
            "UNICODE",                     -- From CMakeLists.txt (MSVC 2015+)
            "_UNICODE",                    -- From CMakeLists.txt (MSVC 2015+)
        }

    -- Unix/Linux settings (matches CMakeLists.txt GCC section)
    filter "system:not windows"
        buildoptions {
            "-fvisibility=hidden",          -- Hide non-exported symbols
            "-fno-strict-aliasing",         -- Aliasing safety
            "-Wall",                        -- All warnings
        }
        defines {
            "_FILE_OFFSET_BITS=64",         -- 64-bit file support (32-bit Linux)
        }

    -- Debug configuration (matches CMakeLists.txt debug settings)
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines { 
            "_DEBUG",
        }

    -- Release configurations (matches CMakeLists.txt release settings)
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        defines { 
            "NDEBUG",
        }

    filter "configurations:Dist"
        runtime "Release"
        optimize "On" 
        defines { 
            "NDEBUG",
        }

    -- MSVC-specific optimizations (from CMakeLists.txt MSVC section)
    filter { "configurations:Release or Dist", "action:vs*" }
        buildoptions {
            "/O2",              -- Optimize for speed
        }

    -- GCC/Clang optimizations (from CMakeLists.txt)
    filter { "configurations:Release or Dist", "action:not vs*" }
        buildoptions {
            "-O3",              -- Maximum optimization
        }

    -- GCC-specific settings (from CMakeLists.txt GCC section)
    filter "toolset:gcc"
        buildoptions {
            "-Wno-dangling-reference",      -- GCC 13+ false positive warnings
        }
        -- Position independent code for non-MinGW GCC
        pic "On"

    -- Clang-specific settings (from CMakeLists.txt Clang section)
    filter "toolset:clang"
        buildoptions {
            "-Wno-deprecated-non-prototype",
            "-Wno-long-long",
        }
        pic "On"

    -- MinGW-specific settings (from CMakeLists.txt MinGW section)
    filter "toolset:mingw"
        buildoptions {
            "-Wa,-mbig-obj",                -- Large object files
            "-Wno-dangling-reference",      -- MinGW warning suppression
        }

    -- Clang-cl specific (clang with MSVC ABI)
    filter { "toolset:clang", "system:windows" }
        buildoptions {
            "/bigobj",          -- Large object files for clang-cl
        }

    filter {}