cmake_minimum_required(VERSION 3.16)
project(spdlog VERSION 1.12.0 LANGUAGES CXX)

# Options
option(SPDLOG_BUILD_SHARED "Build shared library" OFF)
option(SPDLOG_BUILD_EXAMPLE "Build example" OFF)
option(SPDLOG_BUILD_TESTS "Build tests" OFF)
option(SPDLOG_ENABLE_PCH "Build static or shared library using precompiled header to speed up compilation time" OFF)
option(SPDLOG_BUILD_PIC "Build position independent code (-fPIC)" OFF)

# Create library
if(SPDLOG_BUILD_SHARED)
    add_library(spdlog SHARED)
else()
    add_library(spdlog STATIC)
endif()

# Add source files
target_sources(spdlog PRIVATE
    src/spdlog.cpp
    src/stdout_sinks.cpp
    src/color_sinks.cpp
    src/file_sinks.cpp
    src/async.cpp
    src/cfg.cpp
    src/bundled_fmtlib_format.cpp
)

# Include directories
target_include_directories(spdlog
    PUBLIC

        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compile definitions
target_compile_definitions(spdlog PUBLIC SPDLOG_COMPILED_LIB)

if(WIN32)
    target_compile_definitions(spdlog PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Set properties
set_target_properties(spdlog PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ${SPDLOG_BUILD_PIC}
)

# Create alias
add_library(spdlog::spdlog ALIAS spdlog)