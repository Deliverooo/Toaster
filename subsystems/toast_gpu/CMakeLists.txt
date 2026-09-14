set(GPU_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/api.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/frame.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/upload.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/vma_impl.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/allocation.cpp
)

if (TOASTER_BUILD_SHARED_LIBS)
	add_library(ToastGPU SHARED)
else ()
	add_library(ToastGPU STATIC)
endif ()

target_include_directories(ToastGPU PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastGPU PRIVATE ${GPU_SRC})
target_link_libraries(ToastGPU PUBLIC Toaster::ToastLib Toaster::ToastMath Vulkan::Vulkan)

target_include_directories(ToastGPU PRIVATE "$ENV{VULKAN_SDK}/Include")

add_library(Toaster::ToastGPU ALIAS ToastGPU)

if (TOASTER_BUILD_SHARED_LIBS)
	add_compile_definitions(ToastGPU PUBLIC TST_GPU_BUILD_DLL)
	add_compile_definitions(ToastGPU PRIVATE TST_GPU_DLL_EXPORT)
endif ()

if (TOASTER_INSTALL)
	install(TARGETS ToastGPU
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)

endif ()