#include "toast_kernel/input.hpp"

#include <GLFW/glfw3.h>
#include <memory>

namespace toaster::knl
{
	void input_ctx_create(InputContext *p_out_input_ctx, void *p_window_ctx)
	{
		p_out_input_ctx->windowCtx = p_window_ctx;

		p_out_input_ctx->mouseX  = 0.0f;
		p_out_input_ctx->mouseY  = 0.0f;
		p_out_input_ctx->mouseDx = 0.0f;
		p_out_input_ctx->mouseDy = 0.0f;
		p_out_input_ctx->scrollX = 0.0f;
		p_out_input_ctx->scrollY = 0.0f;

		p_out_input_ctx->currentCursorMode = ECursorMode::eNormal;

		p_out_input_ctx->firstMouse = true;
	}

	void input_ctx_destroy(InputContext *p_input_ctx)
	{
		input_clear_states(p_input_ctx);
		p_input_ctx->windowCtx = nullptr;

		p_input_ctx->mouseX            = 0.0f;
		p_input_ctx->mouseY            = 0.0f;
		p_input_ctx->mouseDx           = 0.0f;
		p_input_ctx->mouseDy           = 0.0f;
		p_input_ctx->scrollX           = 0.0f;
		p_input_ctx->scrollY           = 0.0f;
		p_input_ctx->currentCursorMode = ECursorMode::eNormal;
		p_input_ctx->firstMouse        = false;
	}

	bool input_is_key_down(const InputContext *p_input_ctx, EKeyCode p_key_code)
	{
		return TST_LIB_BIT_TEST(p_input_ctx->currentKeyStates, TST_LIB_ENUM_VAL(p_key_code));
	}

	bool input_is_key_pressed(const InputContext *p_input_ctx, EKeyCode p_key_code)
	{
		return TST_LIB_BIT_TEST(p_input_ctx->currentKeyStates, TST_LIB_ENUM_VAL(p_key_code)) && !TST_LIB_BIT_TEST(p_input_ctx->previousKeyStates,
																												  TST_LIB_ENUM_VAL(p_key_code));
	}

	bool input_is_key_released(const InputContext *p_input_ctx, EKeyCode p_key_code)
	{
		return !TST_LIB_BIT_TEST(p_input_ctx->currentKeyStates, TST_LIB_ENUM_VAL(p_key_code)) && TST_LIB_BIT_TEST(p_input_ctx->previousKeyStates,
																												  TST_LIB_ENUM_VAL(p_key_code));
	}

	bool input_is_mouse_button_down(const InputContext *p_input_ctx, EMouseButton p_button)
	{
		return TST_LIB_BIT_TEST(p_input_ctx->currentMouseStates, TST_LIB_ENUM_VAL(p_button));
	}

	bool input_is_mouse_button_pressed(const InputContext *p_input_ctx, EMouseButton p_button)
	{
		return TST_LIB_BIT_TEST(p_input_ctx->currentMouseStates, TST_LIB_ENUM_VAL(p_button)) && !TST_LIB_BIT_TEST(p_input_ctx->previousMouseStates,
																												  TST_LIB_ENUM_VAL(p_button));
	}

	bool input_is_mouse_button_released(const InputContext *p_input_ctx, EMouseButton p_button)
	{
		return !TST_LIB_BIT_TEST(p_input_ctx->currentMouseStates, TST_LIB_ENUM_VAL(p_button)) && TST_LIB_BIT_TEST(p_input_ctx->previousMouseStates,
																												  TST_LIB_ENUM_VAL(p_button));
	}

	ECursorMode input_get_cursor_mode(const InputContext *p_input_ctx)
	{
		return p_input_ctx->currentCursorMode;
	}

	void input_set_cursor_mode(InputContext *p_input_ctx, ECursorMode p_cursor_mode)
	{
		glfwSetInputMode(static_cast<GLFWwindow *>(p_input_ctx->windowCtx), GLFW_CURSOR, GLFW_CURSOR_NORMAL + TST_LIB_ENUM_VAL(p_cursor_mode));
		if (p_cursor_mode == ECursorMode::eDisabled)
		{
			glfwSetInputMode(static_cast<GLFWwindow *>(p_input_ctx->windowCtx), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
		p_input_ctx->currentCursorMode = p_cursor_mode;
	}

	void input_update(InputContext *p_input_ctx)
	{
		memcpy(&p_input_ctx->previousKeyStates, p_input_ctx->currentKeyStates, TST_LIB_BITSET_SIZE(TST_LIB_ENUM_VAL(EKeyCode::eLast)));
		memcpy(&p_input_ctx->previousMouseStates, p_input_ctx->currentMouseStates, TST_LIB_BITSET_SIZE(TST_LIB_ENUM_VAL(EMouseButton::eLast)));

		p_input_ctx->mouseDx = 0.0f;
		p_input_ctx->mouseDy = 0.0f;

		p_input_ctx->scrollX = 0.0f;
		p_input_ctx->scrollY = 0.0f;
	}

	void input_clear_states(InputContext *p_input_ctx)
	{
		TST_LIB_BITSET_RESET(p_input_ctx->currentKeyStates, TST_LIB_ENUM_VAL(EKeyCode::eLast));
		TST_LIB_BITSET_RESET(p_input_ctx->previousKeyStates, TST_LIB_ENUM_VAL(EKeyCode::eLast));

		TST_LIB_BITSET_RESET(p_input_ctx->currentMouseStates, TST_LIB_ENUM_VAL(EMouseButton::eLast));
		TST_LIB_BITSET_RESET(p_input_ctx->previousMouseStates, TST_LIB_ENUM_VAL(EMouseButton::eLast));
	}

	math::Float2 input_get_mouse_pos(const InputContext *p_input_ctx)
	{
		return {p_input_ctx->mouseX, p_input_ctx->mouseY};
	}

	math::Float2 input_get_mouse_delta_pos(const InputContext *p_input_ctx)
	{
		return {p_input_ctx->mouseDx, p_input_ctx->mouseDy};
	}

	math::Float2 input_get_mouse_scroll(const InputContext *p_input_ctx)
	{
		math::Float2 out_scroll;
		out_scroll.x = p_input_ctx->scrollX;
		out_scroll.y = p_input_ctx->scrollY;
		return out_scroll;
	}
}
