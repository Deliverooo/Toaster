cmake_minimum_required(VERSION 3.16)
project(glm VERSION 1.0.1 LANGUAGES CXX)

# GLM is header-only
add_library(glm INTERFACE)

# Include directories - make sure local GLM takes precedence
target_include_directories(glm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard requirement to match your project
target_compile_features(glm INTERFACE cxx_std_14)

# Add compile definitions to prevent conflicts with VulkanSDK GLM
target_compile_definitions(glm INTERFACE
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_RADIANS
)

# Create alias
add_library(glm::glm ALIAS glm)

# Set target properties to ensure our GLM is used first
set_target_properties(glm PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ""  # Don't treat as system includes
    FOLDER "Dependencies"
)