cmake_minimum_required(VERSION 3.16)
project(entt VERSION 2.0.0 LANGUAGES CXX)

# Create header-only interface library
add_library(entt INTERFACE)

# Include directories
target_include_directories(entt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# Remove or comment out the C++ standard requirement to let the parent project decide
# target_compile_features(entt INTERFACE cxx_std_14)

# Create alias
add_library(entt::entt ALIAS entt)