cmake_minimum_required(VERSION 3.30)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Prevent in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif ()

# Project definition
project(Toaster LANGUAGES C CXX)

# these are added to later on.
set(C_WARNINGS)
set(CXX_WARNINGS)

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	set(_WARNINGS
			# warning level:
#			"/W3"
	)
	string(REPLACE ";" " " _WARNINGS "${_WARNINGS}")
	set(C_WARNINGS "${_WARNINGS}")
	set(CXX_WARNINGS "${_WARNINGS}")
	unset(_WARNINGS)
endif ()

# Select C++23 as the standard for C++ projects.
# If C++23 is not available, downgrading to an earlier standard is NOT OK.
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${C_WARNINGS} ${CMAKE_C_FLAGS}")

option(TOASTER_INSTALL "Generate install targets for toaster" ON)
option(TOASTER_BUILD_SHARED_LIBS "Generate install targets for toaster" ON)

add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
add_compile_definitions(NOMINMAX)

add_subdirectory(extern)

set(CMAKE_DEBUG_POSTFIX "d")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_subdirectory(subsystems)

if (TOASTER_INSTALL)
	include(CMakePackageConfigHelpers)

	configure_package_config_file(
			${CMAKE_CURRENT_SOURCE_DIR}/cmake/ToasterConfig.cmake.in
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfig.cmake
			INSTALL_DESTINATION lib/cmake/Toaster
	)

	write_basic_package_version_file(
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfigVersion.cmake
			VERSION 1.0.0
			COMPATIBILITY AnyNewerVersion
	)

	install(FILES
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfig.cmake
			${CMAKE_CURRENT_BINARY_DIR}/ToasterConfigVersion.cmake
			DESTINATION lib/cmake/Toaster
	)

	install(EXPORT ToasterTargets
			FILE ToasterTargets.cmake
			NAMESPACE Toaster::
			DESTINATION lib/cmake/Toaster
	)
endif ()

add_subdirectory(test)