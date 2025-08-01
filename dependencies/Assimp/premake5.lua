project "Assimp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"

    targetdir ("../../bin/" .. outputDir .. "/%{prj.name}")
    objdir ("../../bin-int/" .. outputDir .. "/%{prj.name}")

    files {
        "code/AssetLib/**.cpp",
        "code/AssetLib/**.h",
        "code/Common/**.cpp",
        "code/Common/**.h",
        "code/CApi/**.cpp",
        "code/CApi/**.h",
        "code/Material/**.cpp",
        "code/Material/**.h",
        "code/PostProcessing/**.cpp",
        "code/PostProcessing/**.h",
        "include/assimp/**.h",
        "include/assimp/**.hpp",
        "include/assimp/**.inl",
        "contrib/zlib/*.c",
        "contrib/zlib/*.h",
        "contrib/utf8cpp/source/*.h",
        "contrib/Open3DGC/**.h",
        "contrib/Open3DGC/**.cpp",
        "contrib/unzip/*.c",
        "contrib/unzip/*.h",
        "contrib/poly2tri/poly2tri/**.h",
        "contrib/poly2tri/poly2tri/**.cc",
        "contrib/clipper/*.hpp",
        "contrib/clipper/*.cpp",
        "revision.h"
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
        "contrib/openddlparser/include",
        "contrib/Open3DGC",
        "contrib/poly2tri",
        "contrib/clipper"
    }

defines {
        "ASSIMP_BUILD_NO_EXPORT",
        "ASSIMP_BUILD_NO_C4D_IMPORTER",
        "ASSIMP_BUILD_NO_3DS_IMPORTER",
        "ASSIMP_BUILD_NO_AC_IMPORTER",
        "ASSIMP_BUILD_NO_ASE_IMPORTER",
        "ASSIMP_BUILD_NO_B3D_IMPORTER",
        "ASSIMP_BUILD_NO_BLEND_IMPORTER",
        "ASSIMP_BUILD_NO_BVH_IMPORTER",
        "ASSIMP_BUILD_NO_COB_IMPORTER",
        "ASSIMP_BUILD_NO_COLLADA_IMPORTER",
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
        "ASSIMP_BUILD_NO_STL_IMPORTER",
        "ASSIMP_BUILD_NO_TERRAGEN_IMPORTER",
        "ASSIMP_BUILD_NO_UNREAL_IMPORTER",
        "ASSIMP_BUILD_NO_X_IMPORTER",
        "ASSIMP_BUILD_NO_X3D_IMPORTER",
        "ASSIMP_BUILD_NO_XGL_IMPORTER",
        -- Enable only the formats you need
        -- "ASSIMP_BUILD_NO_FBX_IMPORTER",
        -- "ASSIMP_BUILD_NO_OBJ_IMPORTER",
        -- "ASSIMP_BUILD_NO_GLTF_IMPORTER",
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
        defines {
            "_CRT_SECURE_NO_WARNINGS",
            "_SCL_SECURE_NO_WARNINGS",
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { 
            "_DEBUG",
            "ASSIMP_BUILD_DEBUG"
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        defines { "NDEBUG" }

    filter "configurations:Dist"
        runtime "Release"
        optimize "On"
        defines { "NDEBUG" }

    filter {}