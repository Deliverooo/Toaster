cmake_minimum_required(VERSION 3.16)
project(imgui VERSION 1.0.1 LANGUAGES CXX)

add_library(imgui STATIC)

target_sources(imgui PRIVATE
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

# Fix: Make sure headers are properly exposed
target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    glad  # Add glad dependency since OpenGL3 backend needs it
)

# Set properties
set_target_properties(imgui PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    FOLDER "dependencies"
)
