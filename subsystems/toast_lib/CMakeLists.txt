set(SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/toast_assert.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/string.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/memory.cpp
)

if (TOASTER_BUILD_SHARED_LIBS)
	add_library(ToastLib SHARED)
else ()
	add_library(ToastLib STATIC)
endif ()

target_include_directories(ToastLib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastLib PRIVATE ${SRC})

add_library(Toaster::ToastLib ALIAS ToastLib)

if (TOASTER_BUILD_SHARED_LIBS)
	add_compile_definitions(ToastLib PUBLIC TST_LIB_BUILD_DLL)
	add_compile_definitions(ToastLib PRIVATE TST_LIB_DLL_EXPORT)
endif ()

if (TOASTER_INSTALL)
	install(TARGETS ToastLib
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)
endif ()