set(RENDER_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/texture_importer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/mesh_importer.cpp
)

add_library(ToastAsset STATIC) # Asset has to be static to avoid inter-dll dependencies

target_include_directories(ToastAsset PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastAsset PRIVATE ${RENDER_SRC})
target_link_libraries(ToastAsset PUBLIC Toaster::ToastScene assimp::assimp stb)

add_library(Toaster::ToastAsset ALIAS ToastAsset)

if (TOASTER_INSTALL)
	install(TARGETS ToastAsset
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)
endif ()