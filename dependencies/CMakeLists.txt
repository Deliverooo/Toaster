# Set all dependencies to not build examples/tests by default
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the Glfw Examples?" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "Build the Glfw Tests?" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "Build the Glfw Documentation?" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

set(SPDLOG_BUILD_EXAMPLE    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED     OFF CACHE BOOL "" FORCE)

# Configure Assimp
set(ASSIMP_BUILD_TESTS          OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS   OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES        OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL              OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS           OFF CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

# set(ASSIMP_BUILD_NO_X_IMPORTER ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_NO_3DS_IMPORTER ON CACHE BOOL "" FORCE)

add_subdirectory(Glad)
add_subdirectory(GLFW)
add_subdirectory(spdlog)
add_subdirectory(stbImage)
add_subdirectory(glm)
add_subdirectory(Entt)
add_subdirectory(Tinyobjloader)
add_subdirectory(ImGui)
add_subdirectory(Assimp)
add_subdirectory(YamlCpp)


if(TARGET glfw)
    set_target_properties(glfw PROPERTIES FOLDER "dependencies")
endif()

if(TARGET glad)
    set_target_properties(glad PROPERTIES FOLDER "dependencies")
endif()

if(TARGET imgui)
    set_target_properties(imgui PROPERTIES FOLDER "dependencies")
endif()

if(TARGET spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER "dependencies")
endif()

if(TARGET stbImage)
    set_target_properties(stbImage PROPERTIES FOLDER "dependencies")
endif()

if(TARGET glm)
    set_target_properties(glm PROPERTIES FOLDER "dependencies")
endif()

if(TARGET tinyobjloader)
    set_target_properties(tinyobjloader PROPERTIES FOLDER "dependencies")
endif()

if(TARGET entt)
    set_target_properties(entt PROPERTIES FOLDER "dependencies")
endif()

if(TARGET assimp)
    set_target_properties(assimp PROPERTIES FOLDER "dependencies")
endif()

if(TARGET YAML_CPP)
    set_target_properties(YAML_CPP PROPERTIES FOLDER "dependencies")
endif()

if(TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "dependencies")
endif()
