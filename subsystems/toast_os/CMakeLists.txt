set(SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/console.cpp
)


if (TOASTER_BUILD_SHARED_LIBS)
	add_library(ToastOS SHARED)
else ()
	add_library(ToastOS STATIC)
endif ()

target_include_directories(ToastOS PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastOS PRIVATE ${SRC})

add_library(Toaster::ToastOS ALIAS ToastOS)

if (TOASTER_BUILD_SHARED_LIBS)
	add_compile_definitions(ToastOS PUBLIC TST_OS_BUILD_DLL)
	add_compile_definitions(ToastOS PRIVATE TST_OS_DLL_EXPORT)
endif ()

if (TOASTER_INSTALL)
	install(TARGETS ToastOS
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)

endif ()