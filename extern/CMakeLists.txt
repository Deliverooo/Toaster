add_subdirectory(stb)

include(FetchContent)

#Mimalloc ---
set(MI_INSTALL_TOPLEVEL ON CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC OFF CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MI_WIN_REDIRECT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
		mimalloc
		GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
		GIT_TAG main3
)
FetchContent_MakeAvailable(mimalloc)
#Mimalloc ---

#Vulkan Headers ---
set(VULKAN_HEADERS_ENABLE_MODULE OFF CACHE BOOL "" FORCE)
set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		vulkan_headers
		GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
		GIT_TAG main
)
FetchContent_MakeAvailable(vulkan_headers)
#Vulkan Headers ---

message("Vulkan headers install dir: ${vulkan_headers_SOURCE_DIR}")

set(VULKAN_HEADERS_INSTALL_DIR ${vulkan_headers_SOURCE_DIR})

#Vma Headers ---
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		vma_headers
		GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
		GIT_TAG master
)
FetchContent_MakeAvailable(vma_headers)
#Vma Headers ---

#Volk ---
FetchContent_Declare(
		volk
		GIT_REPOSITORY https://github.com/zeux/volk.git
		GIT_TAG master
)
FetchContent_MakeAvailable(volk)
#Volk ---

#FastGLTF ---
FetchContent_Declare(
		fastgltf
		GIT_REPOSITORY https://github.com/spnda/fastgltf.git
		GIT_TAG main
)
FetchContent_MakeAvailable(fastgltf)
#FastGLTF ---

#Meshoptimizer ---
set(MESHOPT_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
		meshopt
		GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
		GIT_TAG master
)
FetchContent_MakeAvailable(meshopt)
#Meshoptimizer ---

#GLFW ---
set(GLFW_INSTALL ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
		glfw
		GIT_REPOSITORY https://github.com/glfw/glfw.git
		GIT_TAG master
)
FetchContent_MakeAvailable(glfw)
#GLFW ---

#EnTT ---
set(ENTT_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
		entt
		GIT_REPOSITORY https://github.com/skypjack/entt.git
		GIT_TAG main
)
FetchContent_MakeAvailable(entt)
#EnTT ---

#Taskflow ---
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		taskflow
		GIT_REPOSITORY https://github.com/taskflow/taskflow.git
		GIT_TAG master
)
FetchContent_MakeAvailable(taskflow)
#Taskflow ---

##Jolt ---
#set(JPH_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
#set(ENABLE_INSTALL ON CACHE BOOL "" FORCE)
#FetchContent_Declare(
#		jolt
#		GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
#		GIT_TAG master
#		SOURCE_SUBDIR "Build"
#)
#FetchContent_MakeAvailable(jolt)
##Jolt ---