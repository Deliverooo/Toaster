set(RENDER_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/transform.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/render_context.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/texture.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/mesh.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/material.cpp
)

add_library(ToastRender SHARED)
add_library(Toaster::ToastRender ALIAS ToastRender)

target_include_directories(ToastRender PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(ToastRender PRIVATE ${RENDER_SRC})
target_link_libraries(ToastRender PUBLIC Toaster::ToastGPU)

add_compile_definitions(ToastRender PUBLIC TST_RENDER_BUILD_DLL)
add_compile_definitions(ToastRender PRIVATE TST_RENDER_DLL_EXPORT)

if (TOASTER_INSTALL)
	install(TARGETS ToastRender
			EXPORT ToasterTargets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()