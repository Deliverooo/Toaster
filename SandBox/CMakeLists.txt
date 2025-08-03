# SandBox executable

# Source files
file(GLOB_RECURSE SANDBOX_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/*.hpp"
)

# Create the executable
add_executable(SandBox ${SANDBOX_SOURCES})

# Include directories
target_include_directories(SandBox PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/Toaster/src
)

# Link libraries
target_link_libraries(SandBox PRIVATE
    Toaster  # Link with Toaster library
)

# Set properties
set_target_properties(SandBox PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    FOLDER "Applications"
)

# Platform-specific settings
if(WIN32)
    target_compile_options(SandBox PRIVATE /utf-8)
    target_compile_definitions(SandBox PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        TST_PLATFORM_WINDOWS
        TST_SANDBOX_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res"
    )
endif()