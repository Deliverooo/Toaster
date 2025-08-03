add_library(Toaster STATIC)
add_library(Toaster::Toaster ALIAS Toaster)

# Source files
file(GLOB_RECURSE TOASTER_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "src/*.hpp"
)

target_sources(Toaster PRIVATE ${TOASTER_SOURCES})

# Precompiled headers
target_precompile_headers(Toaster PRIVATE "src/tstpch.h")

# Include directories
target_include_directories(Toaster PUBLIC
    src
    ${CMAKE_SOURCE_DIR}/dependencies/GLFW/include
    ${CMAKE_SOURCE_DIR}/dependencies/Glad/include
    ${CMAKE_SOURCE_DIR}/dependencies/ImGui
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/stbImage/include
    ${CMAKE_SOURCE_DIR}/dependencies/glm
    ${CMAKE_SOURCE_DIR}/dependencies/Entt/include
    ${CMAKE_SOURCE_DIR}/dependencies/Tinyobjloader/include
    ${CMAKE_SOURCE_DIR}/dependencies/Assimp/include
)

# Link libraries - spdlog should be PUBLIC since it's used in headers
target_link_libraries(Toaster
    PUBLIC
        glm::glm
        imgui
        entt::entt
        spdlog::spdlog      # Move spdlog to PUBLIC since Log.hpp includes spdlog headers
        assimp
    PRIVATE
        glfw
        glad
        stbImage
        tinyobjloader::tinyobjloader
        ${Vulkan_LIBRARIES}
        $<$<PLATFORM_ID:Windows>:opengl32>
        $<$<PLATFORM_ID:Windows>:dwmapi>
)

# Manually add Vulkan includes (without GLM path)
if(Vulkan_INCLUDE_DIRS)
    target_include_directories(Toaster PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

# Compile definitions
target_compile_definitions(Toaster PRIVATE
    SPDLOG_COMPILED_LIB
    $<$<CONFIG:Debug>:TST_DEBUG>
    $<$<CONFIG:Release>:TST_RELEASE>
    $<$<CONFIG:Dist>:TST_DIST>
    $<$<PLATFORM_ID:Windows>:TST_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Windows>:TST_BUILD_DLL>
    $<$<PLATFORM_ID:Windows>:GLFW_INCLUDE_NONE>
    TST_ENABLE_FBX
    TST_CORE_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res"
)

# Platform-specific settings
if(WIN32)
    target_compile_options(Toaster PRIVATE /utf-8)
    target_compile_definitions(Toaster PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Set properties
set_target_properties(Toaster PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    FOLDER "Engine"
)