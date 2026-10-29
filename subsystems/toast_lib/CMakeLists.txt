set(SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/toast_assert.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/memory.cpp
)

add_library(ToastLib STATIC) # There is no reason why Toast Lib should be shared
add_library(Toaster::ToastLib ALIAS ToastLib)

target_include_directories(ToastLib PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(ToastLib PRIVATE ${SRC})
target_link_libraries(ToastLib PUBLIC rpmalloc::rpmalloc)

if (TOASTER_INSTALL)
	install(TARGETS ToastLib
			EXPORT ToasterTargets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
	install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()