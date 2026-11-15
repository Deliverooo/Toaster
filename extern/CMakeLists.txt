include(FetchContent)

#rpmalloc ---
FetchContent_Declare(
		rpmalloc
		URL https://github.com/mjansson/rpmalloc/archive/refs/heads/develop.zip
)
FetchContent_MakeAvailable(rpmalloc)

add_library(rpmalloc SHARED)
add_library(rpmalloc::rpmalloc ALIAS rpmalloc)

target_sources(rpmalloc PRIVATE "${rpmalloc_SOURCE_DIR}/rpmalloc/rpmalloc.c")
target_include_directories(rpmalloc PUBLIC
		$<BUILD_INTERFACE:${rpmalloc_SOURCE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(rpmalloc PRIVATE
		ENABLE_STATISTICS=1
		ENABLE_ASSERTS=0
)

set_target_properties(rpmalloc PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (TOASTER_INSTALL)
	install(TARGETS rpmalloc
			EXPORT ToasterTargets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
	install(DIRECTORY ${rpmalloc_SOURCE_DIR}/rpmalloc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
			PATTERN "*.c" EXCLUDE
			PATTERN "rpnew.h" EXCLUDE # This can only be included in one source file
	)
endif ()
#rpmalloc ---

add_subdirectory(stb)

#Vulkan Headers ---
set(VULKAN_HEADERS_ENABLE_MODULE OFF CACHE BOOL "" FORCE)
set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		vulkan_headers
		URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(vulkan_headers)
#Vulkan Headers ---

set(VULKAN_HEADERS_INSTALL_DIR ${vulkan_headers_SOURCE_DIR})

#Vma Headers ---
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		vma_headers
		URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(vma_headers)
#Vma Headers ---

#Volk ---
FetchContent_Declare(
		volk
		URL https://github.com/zeux/volk/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(volk)
#Volk ---

#GLFW ---
set(GLFW_INSTALL ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
		glfw
		URL https://github.com/glfw/glfw/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(glfw)
#GLFW ---

#KTX ---
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_JS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
		ktx
		GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
		GIT_TAG main
)
FetchContent_MakeAvailable(ktx)
#KTX ---