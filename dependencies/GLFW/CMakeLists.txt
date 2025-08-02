cmake_minimum_required(VERSION 3.16)
project(glfw VERSION 3.5.0 LANGUAGES C)

# Options tailored for your project
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)

# Platform detection
if(WIN32)
    set(_GLFW_WIN32 1)
elseif(APPLE)
    set(_GLFW_COCOA 1)
elseif(UNIX)
    set(_GLFW_X11 1)
endif()

# Create the GLFW library
add_library(glfw STATIC)

# Add source files - matches your GLFW 3.5 structure
target_sources(glfw PRIVATE
    include/GLFW/glfw3.h
    include/GLFW/glfw3native.h
    src/context.c
    src/init.c
    src/input.c
    src/monitor.c
    src/platform.c
    src/vulkan.c
    src/window.c
    src/egl_context.c
    src/osmesa_context.c
    src/null_init.c
    src/null_joystick.c
    src/null_monitor.c
    src/null_window.c
)

# Platform-specific sources for Windows (your target platform)
if(_GLFW_WIN32)
    target_sources(glfw PRIVATE
        src/win32_init.c
        src/win32_joystick.c
        src/win32_module.c
        src/win32_monitor.c
        src/win32_time.c
        src/win32_thread.c
        src/win32_window.c
        src/wgl_context.c
    )
    target_compile_definitions(glfw PRIVATE _GLFW_WIN32)
    target_link_libraries(glfw PRIVATE gdi32)
endif()

# Include directories
target_include_directories(glfw
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Set target properties
set_target_properties(glfw PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Create alias for compatibility
add_library(glfw3 ALIAS glfw)