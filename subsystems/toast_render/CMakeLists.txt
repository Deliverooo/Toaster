set(RENDER_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/transform_system.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/render_context.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/texture.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/mesh.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/material.cpp
)

if (TOASTER_BUILD_SHARED_LIBS)
	add_library(ToastRender SHARED)
else ()
	add_library(ToastRender STATIC)
endif ()

target_include_directories(ToastRender PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastRender PRIVATE ${RENDER_SRC})
target_link_libraries(ToastRender PUBLIC Toaster::ToastGPU)

add_library(Toaster::ToastRender ALIAS ToastRender)

if (TOASTER_BUILD_SHARED_LIBS)
	add_compile_definitions(ToastRender PUBLIC TST_RENDER_BUILD_DLL)
	add_compile_definitions(ToastRender PRIVATE TST_RENDER_DLL_EXPORT)
endif ()

if (TOASTER_INSTALL)
	install(TARGETS ToastRender
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)

endif ()