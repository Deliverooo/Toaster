set(RENDER_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/scene.cpp
)

add_library(ToastScene STATIC)  # Scene has to be static

target_include_directories(ToastScene PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastScene PRIVATE ${RENDER_SRC})

target_link_libraries(ToastScene PUBLIC Toaster::ToastAsset Toaster::ToastRender EnTT::EnTT)

add_library(Toaster::ToastScene ALIAS ToastScene)

if (TOASTER_INSTALL)
	install(TARGETS ToastScene
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)
endif ()