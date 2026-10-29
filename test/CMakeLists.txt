set(SRC
		main.cpp
)

add_executable(toaster_test WIN32 ${SRC})

target_link_libraries(toaster_test PRIVATE
		Toaster::ToastLib
		Toaster::ToastOS
		Toaster::ToastKernel
		Toaster::ToastAsset
)

find_package(Vulkan REQUIRED COMPONENTS dxc glslangValidator shaderc_combined SPIRV-Tools)

set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shader_headers)
file(MAKE_DIRECTORY ${SHADER_GEN_DIR})

set(SHADER_SOURCES_GLSL
		"${SHADER_SRC_DIR}/test.mesh"
		"${SHADER_SRC_DIR}/test.frag"
)

set(GENERATED_HEADERS "")
foreach (SHADER_PATH IN LISTS SHADER_SOURCES_GLSL)

	get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)

	set(HEADER_PATH "${SHADER_GEN_DIR}/${SHADER_NAME}.h")
	list(APPEND GENERATED_HEADERS ${HEADER_PATH})

	string(REPLACE "." "_" VARIABLE_NAME "${SHADER_NAME}")

	add_custom_command(
			OUTPUT ${HEADER_PATH}
			COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V --target-env vulkan1.4 --vn "c_${VARIABLE_NAME}_bytecode" -o ${HEADER_PATH} ${SHADER_PATH}
			DEPENDS ${SHADER_PATH} "${SHADER_SRC_DIR}/common.glsl"
			COMMENT "Compiling shader: ${SHADER_NAME}"
			VERBATIM
	)
endforeach ()

add_custom_target(TestShaders DEPENDS ${GENERATED_HEADERS})

add_dependencies(toaster_test TestShaders)
target_include_directories(toaster_test PRIVATE ${SHADER_GEN_DIR})
