cmake_minimum_required(VERSION 3.16)
project(tinyobjloader VERSION 2.0.0 LANGUAGES CXX)

# Create header-only interface library
add_library(tinyobjloader INTERFACE)

# Include directories
target_include_directories(tinyobjloader INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set C++ standard requirement
target_compile_features(tinyobjloader INTERFACE cxx_std_17)

# Create alias
add_library(tinyobjloader::tinyobjloader ALIAS tinyobjloader)