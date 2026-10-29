cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" FORCE) # The libraries will look like "ToastGPUd.dll"

# Prevent in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin" CACHE STRING "" FORCE) # .exe
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin" CACHE STRING "" FORCE) # .dll
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib" CACHE STRING "" FORCE) # .lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "" FORCE)

# Project definition
project(Toaster LANGUAGES C CXX)

set(CXX_FLAGS)

if (MSVC)
	set(CXX_FLAGS
			"/W3"
	)
	add_compile_options("/experimental:c11atomics")
	add_compile_definitions(_HAS_EXCEPTIONS=0)
endif ()

set(CMAKE_CXX_FLAGS ${CXX_FLAGS})

option(TOASTER_INSTALL "Generate install targets for toaster" ON)
option(TOASTER_BUILD_SHARED_LIBS "Generate install targets for toaster" ON)

add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
add_compile_definitions(NOMINMAX)


include(GNUInstallDirs) # Allow subsystems to access CMAKE_INSTALL_XXX variables

add_subdirectory(extern)
add_subdirectory(subsystems)

if (TOASTER_INSTALL)
	message(NOTICE "Installation include directory: ${CMAKE_INSTALL_INCLUDEDIR}")
	message(NOTICE "Installation binary directory: ${CMAKE_INSTALL_BINDIR}")
	message(NOTICE "Installation library directory: ${CMAKE_INSTALL_LIBDIR}")

	include(CMakePackageConfigHelpers)

	configure_package_config_file(
			${CMAKE_CURRENT_SOURCE_DIR}/cmake/ToasterConfig.cmake.in
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfig.cmake
			INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Toaster
	)

	write_basic_package_version_file(
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfigVersion.cmake
			VERSION 3.0.0
			COMPATIBILITY SameMajorVersion
	)

	install(FILES
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfig.cmake
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfigVersion.cmake
			DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Toaster
	)

	install(EXPORT ToasterTargets
			FILE ToasterTargets.cmake
			NAMESPACE Toaster::
			DESTINATION lib/cmake/Toaster
	)
endif ()

add_subdirectory(test)