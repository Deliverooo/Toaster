set(GPU_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/api.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/frame.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/upload.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/vma_impl.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/allocation.cpp
)

add_library(ToastGPU SHARED)
add_library(Toaster::ToastGPU ALIAS ToastGPU)

target_include_directories(ToastGPU PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(ToastGPU PRIVATE ${GPU_SRC})

target_link_libraries(ToastGPU PUBLIC Toaster::ToastLib Toaster::ToastMath)
target_link_libraries(ToastGPU PRIVATE Vulkan::Headers volk::volk VulkanMemoryAllocator)

add_compile_definitions(ToastGPU PRIVATE VK_NO_PROTOTYPES)
add_compile_definitions(ToastGPU PRIVATE VK_USE_PLATFORM_WIN32_KHR)

add_compile_definitions(ToastGPU PUBLIC TST_GPU_BUILD_DLL)
add_compile_definitions(ToastGPU PRIVATE TST_GPU_DLL_EXPORT)

if (TOASTER_INSTALL)
	install(TARGETS ToastGPU
			EXPORT ToasterTargets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()