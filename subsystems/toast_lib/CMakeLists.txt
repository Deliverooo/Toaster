set(SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/toast_assert.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/memory.cpp
)

add_library(ToastLib STATIC) # There is no reason why Toast Lib should be shared

target_include_directories(ToastLib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_sources(ToastLib PRIVATE ${SRC})
target_link_libraries(ToastLib PUBLIC mimalloc)

add_library(Toaster::ToastLib ALIAS ToastLib)

if (TOASTER_INSTALL)
	install(TARGETS ToastLib
			EXPORT ToasterTargets
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
	install(DIRECTORY include/ DESTINATION include)
endif ()