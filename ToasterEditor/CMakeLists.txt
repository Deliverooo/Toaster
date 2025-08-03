# ToasterEditor executable

# Source files
file(GLOB_RECURSE TOASTEREDITOR_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/*.hpp"
)

# Create the executable
add_executable(ToasterEditor ${TOASTEREDITOR_SOURCES})

# Include directories
target_include_directories(ToasterEditor PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/Toaster/src
)

# Link libraries - This is the key fix
target_link_libraries(ToasterEditor PRIVATE
    Toaster  # Link with Toaster library (not Toaster::Toaster)
)

# Set properties
set_target_properties(ToasterEditor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    FOLDER "Applications"
)

# Platform-specific settings
if(WIN32)
    target_compile_options(ToasterEditor PRIVATE /utf-8)
    target_compile_definitions(ToasterEditor PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        TST_PLATFORM_WINDOWS
        TST_EDITOR_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res"
    )
endif()