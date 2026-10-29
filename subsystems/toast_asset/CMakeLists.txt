set(RENDER_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/texture_importer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/mesh_importer.cpp
)

add_library(ToastAsset SHARED)
add_library(Toaster::ToastAsset ALIAS ToastAsset)

target_include_directories(ToastAsset PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(ToastAsset PRIVATE ${RENDER_SRC})

target_link_libraries(ToastAsset PUBLIC Toaster::ToastRender)
target_link_libraries(ToastAsset PRIVATE fastgltf::fastgltf meshoptimizer stb)

add_compile_definitions(ToastAsset PUBLIC TST_ASSET_BUILD_DLL)
add_compile_definitions(ToastAsset PRIVATE TST_ASSET_DLL_EXPORT)

if (TOASTER_INSTALL)
	install(TARGETS ToastAsset
			EXPORT ToasterTargets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()