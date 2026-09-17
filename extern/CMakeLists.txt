add_subdirectory(stb)

include(FetchContent)

#GLFW ---
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