set(KERNEL_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/input.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/layer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/camera.cpp
)

add_library(ToastKernel SHARED)

target_include_directories(ToastKernel PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastKernel PRIVATE ${KERNEL_SRC})

target_link_libraries(ToastKernel PUBLIC Toaster::ToastRender)
target_link_libraries(ToastKernel PRIVATE dwmapi glfw)

add_library(Toaster::ToastKernel ALIAS ToastKernel)

add_compile_definitions(ToastKernel PUBLIC TST_KERNEL_BUILD_DLL)
add_compile_definitions(ToastKernel PRIVATE TST_KERNEL_DLL_EXPORT)

if (TOASTER_INSTALL)
	install(TARGETS ToastKernel
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)
endif ()