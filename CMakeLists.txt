cmake_minimum_required(VERSION 3.30)

set(CMAKE_DEBUG_POSTFIX "d")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
# Prevent in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif ()

# Project definition
project(Toaster VERSION 3.0.0)

find_package(Vulkan REQUIRED COMPONENTS dxc glslangValidator shaderc_combined SPIRV-Tools)

if (NOT ${Vulkan_FOUND} OR NOT ${Vulkan_glslangValidator_FOUND} OR NOT ${Vulkan_shaderc_combined_FOUND} OR NOT ${Vulkan_SPIRV-Tools_FOUND})
	message(FATAL_ERROR "Vulkan enabled as a graphics backend, but cannot be found or cannot find required components!")
	message(FATAL_ERROR "Try (re)installing the Vulkan SDK and checking that the shader components are selected")
else ()
	message(NOTICE "Found Vulkan :)")
endif ()

# these are added to later on.
set(C_WARNINGS)
set(CXX_WARNINGS)

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	set(_WARNINGS
			# warning level:
			"/W3"
			"/w34062"  # switch statement contains 'default' but no 'case' labels
			"/w34100"  # 'identifier' : unreferenced formal parameter
			"/w34115"  # 'type' : named type definition in parentheses
			"/w34189"  # local variable is initialized but not referenced
			# see https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5038?view=vs-2017
			"/w35038"  # order of initialization in c++ constructors
			# disable:
			"/wd4018"  # signed/unsigned mismatch
			"/wd4146"  # unary minus operator applied to unsigned type, result still unsigned
			"/wd4065"  # switch statement contains 'default' but no 'case' labels
			"/wd4127"  # conditional expression is constant
			"/wd4181"  # qualifier applied to reference type; ignored
			"/wd4200"  # zero-sized array in struct/union
			"/wd4244"  # conversion from 'type1' to 'type2', possible loss of data
			"/wd4267"  # conversion from 'size_t' to 'type', possible loss of data
			"/wd4305"  # truncation from 'type1' to 'type2'
			"/wd4800"  # forcing value to bool 'true' or 'false'
			"/wd4828"  # The file contains a character that is illegal
			"/wd4996"  # identifier was declared deprecated
			"/wd4661"  # no suitable definition provided for explicit template instantiation request
			"/wd4848"  # 'no_unique_address' is a vendor extension in C++17
			"/wd4251"
			# errors:
			"/we4013"  # 'function' undefined; assuming extern returning int
			"/we4133"  # incompatible pointer types
			"/we4431"  # missing type specifier - int assumed
			"/we4033"  # 'function' must return a value
	)
	string(REPLACE ";" " " _WARNINGS "${_WARNINGS}")
	set(C_WARNINGS "${_WARNINGS}")
	set(CXX_WARNINGS "${_WARNINGS}")
	unset(_WARNINGS)
endif ()

# Select C++23 as the standard for C++ projects.
# If C++23 is not available, downgrading to an earlier standard is NOT OK.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${C_WARNINGS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CXX_WARNINGS} ${CMAKE_CXX_FLAGS}")

option(TOASTER_INSTALL "Generate install targets for toaster" ON)
option(TOASTER_BUILD_SHARED_LIBS "Generate install targets for toaster" ON)

add_subdirectory(extern)
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