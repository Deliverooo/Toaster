#include "toast_kernel/input.hpp"
#include "toast_kernel/window.hpp"

#include <GLFW/glfw3.h>

namespace toaster::ke
{
	auto input_ctx_create(InputContext *p_out_input_ctx, void *p_window_ctx) -> void
	{
		p_out_input_ctx->windowCtx = p_window_ctx;
	}

	auto input_ctx_destroy(InputContext *p_input_ctx) -> void
	{
		input_ctx_clear_states(p_input_ctx);
		p_input_ctx->windowCtx = nullptr;

		p_input_ctx->mouseX            = 0.0f;
		p_input_ctx->mouseY            = 0.0f;
		p_input_ctx->mouseDx           = 0.0f;
		p_input_ctx->mouseDy           = 0.0f;
		p_input_ctx->scrollX           = 0.0f;
		p_input_ctx->scrollY           = 0.0f;
		p_input_ctx->currentCursorMode = ECursorMode{0u};
		p_input_ctx->firstMouse        = false;
	}

	auto input_ctx_is_key_down(const InputContext *p_input_ctx, EKeyCode p_key_code) -> bool
	{
		return p_input_ctx->currentKeyStates[static_cast<uint32>(p_key_code)];
	}

	auto input_ctx_is_key_pressed(const InputContext *p_input_ctx, EKeyCode p_key_code) -> bool
	{
		return p_input_ctx->currentKeyStates[static_cast<uint32>(p_key_code)] && !p_input_ctx->previousKeyStates[static_cast<uint32>(p_key_code)];
	}

	auto input_ctx_is_key_released(const InputContext *p_input_ctx, EKeyCode p_key_code) -> bool
	{
		return !p_input_ctx->currentKeyStates[static_cast<uint32>(p_key_code)] && p_input_ctx->previousKeyStates[static_cast<uint32>(p_key_code)];
	}

	auto input_ctx_is_mouse_button_down(const InputContext *p_input_ctx, EMouseButton p_button) -> bool
	{
		return p_input_ctx->currentMouseStates[static_cast<uint32>(p_button)];
	}

	auto input_ctx_is_mouse_button_pressed(const InputContext *p_input_ctx, EMouseButton p_button) -> bool
	{
		return p_input_ctx->currentMouseStates[static_cast<uint32>(p_button)] && !p_input_ctx->previousMouseStates[static_cast<uint32>(p_button)];
	}

	auto input_ctx_is_mouse_button_released(const InputContext *p_input_ctx, EMouseButton p_button) -> bool
	{
		return !p_input_ctx->currentMouseStates[static_cast<uint32>(p_button)] && p_input_ctx->previousMouseStates[static_cast<uint32>(p_button)];
	}

	auto input_ctx_get_cursor_mode(const InputContext *p_input_ctx) -> ECursorMode
	{
		return p_input_ctx->currentCursorMode;
	}

	auto input_ctx_set_cursor_mode(InputContext *p_input_ctx, ECursorMode p_cursor_mode) -> void
	{
		glfwSetInputMode(static_cast<GLFWwindow *>(p_input_ctx->windowCtx), GLFW_CURSOR, GLFW_CURSOR_NORMAL + static_cast<int32>(p_cursor_mode));
		p_input_ctx->currentCursorMode = p_cursor_mode;
	}

	auto input_ctx_update(InputContext *p_input_ctx) -> void
	{
		p_input_ctx->previousKeyStates   = p_input_ctx->currentKeyStates;
		p_input_ctx->previousMouseStates = p_input_ctx->currentMouseStates;

		p_input_ctx->mouseDx = 0.0f;
		p_input_ctx->mouseDy = 0.0f;

		p_input_ctx->scrollX = 0.0f;
		p_input_ctx->scrollY = 0.0f;
	}

	auto input_ctx_clear_states(InputContext *p_input_ctx) -> void
	{
		p_input_ctx->currentKeyStates.reset();
		p_input_ctx->previousKeyStates.reset();

		p_input_ctx->currentMouseStates.reset();
		p_input_ctx->previousMouseStates.reset();
	}

	auto input_ctx_get_mouse_pos(const InputContext *p_input_ctx) -> tsm::float2
	{
		return {p_input_ctx->mouseX, p_input_ctx->mouseY};
	}

	auto input_ctx_get_mouse_delta_pos(const InputContext *p_input_ctx) -> tsm::float2
	{
		return {p_input_ctx->mouseDx, p_input_ctx->mouseDy};
	}

	auto input_ctx_get_mouse_scroll(const InputContext *p_input_ctx) -> tsm::float2
	{
		return {p_input_ctx->scrollX, p_input_ctx->scrollY};
	}
}
