add_subdirectory(stb)

include(FetchContent)

#Assimp ---
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		assimp
		GIT_REPOSITORY https://github.com/assimp/assimp.git
		GIT_TAG master
)
FetchContent_MakeAvailable(assimp)
#Assimp ---

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

#Jolt ---
set(JPH_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
		jolt
		GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
		GIT_TAG master
		SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(jolt)
#Jolt ---