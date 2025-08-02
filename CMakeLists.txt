cmake_minimum_required(VERSION 3.16)
project(Toaster VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/lib)

# Configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "" FORCE)

add_compile_definitions(
    $<$<CONFIG:Debug>:TST_DEBUG>
    $<$<CONFIG:Release>:TST_RELEASE>
    $<$<CONFIG:Dist>:TST_DIST>
)

# Function to safely add UTF-8 support
function(add_utf8_support target_name)
    if(WIN32 AND TARGET ${target_name})
        # Check if the target already has source-charset or utf-8 options
        get_target_property(CURRENT_OPTIONS ${target_name} COMPILE_OPTIONS)
        if(CURRENT_OPTIONS)
            string(FIND "${CURRENT_OPTIONS}" "source-charset" HAS_SOURCE_CHARSET)
            string(FIND "${CURRENT_OPTIONS}" "/utf-8" HAS_UTF8)
            if(HAS_SOURCE_CHARSET EQUAL -1 AND HAS_UTF8 EQUAL -1)
                target_compile_options(${target_name} PRIVATE /utf-8)
            endif()
        else()
            target_compile_options(${target_name} PRIVATE /utf-8)
        endif()
    endif()
endfunction()

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Find Vulkan AFTER our GLM is established
find_package(Vulkan REQUIRED)

# Manually exclude GLM paths from Vulkan includes
if(Vulkan_INCLUDE_DIRS)
    list(FILTER Vulkan_INCLUDE_DIRS EXCLUDE REGEX "[gG][lL][mM]")
endif()

# Add main projects
add_subdirectory(dependencies)
add_subdirectory(Toaster)
add_subdirectory(SandBox)
add_subdirectory(ToasterEditor)

# Apply UTF-8 support safely to targets that need it
add_utf8_support(spdlog)
add_utf8_support(Toaster)
add_utf8_support(SandBox)
add_utf8_support(ToasterEditor)