#pragma once

#include "input.hpp"
#include "events/event.hpp"

#include "toast_gpu/api.hpp"

#include "toast_math/math_vector.hpp"

struct GLFWwindow;

namespace toaster::ke
{
	struct TST_KERNEL_API WindowDesc
	{
		tsm::uint2 size{1920u, 1080u};
		CString    title{"Toaster-3.0"};
		bool       startMaximized{false};
	};

	enum class EWindowStateFlagBits : uint8
	{
		eResized = TST_BIT(0u), eMinimised = TST_BIT(1u), eMaximised = TST_BIT(1u)
	};

	TST_SPECIALISE_FLAGS(EWindowStateFlagBits, EWindowStateFlags);

	struct TST_KERNEL_API Window
	{
		GLFWwindow *window{nullptr};

		// The exact data that is passed as a user data pointer to the wnd proc
		struct CallbackData
		{
			InputContext inputCtx;

			EventCallbackFn eventCallback{nullptr};
			void *          eventCallbackUserData{nullptr};

			uint32 width{0u};
			uint32 height{0u};

			EWindowStateFlags stateFlags{0u};
		};

		CallbackData cbData{};

		gpu::SurfaceHandle   surface{nullptr};
		gpu::SwapchainHandle swapchain{nullptr};

		gpu::TextureHandle currentTexture{nullptr};
	};

	// These just initialise and shutdown GLFW...
	auto TST_KERNEL_API window_init_api() -> void;
	auto TST_KERNEL_API window_shutdown_api() -> void;

	auto TST_KERNEL_API window_create(Window *p_out_window, const WindowDesc &p_desc) -> void;
	auto TST_KERNEL_API window_destroy(Window *p_window) -> void;

	// Returns true if the window should close
	auto TST_KERNEL_API window_process_messages(const Window *p_window) -> bool;
	// returns false if the window has resized and now the current frame needs to be skipped
	auto TST_KERNEL_API window_check_for_resize(Window *p_window) -> bool;

	// Returns false if the application should skip rendering the current frame
	auto TST_KERNEL_API window_begin_frame(Window *p_window, gpu::CommandListHandle p_cmd) -> bool;
	auto TST_KERNEL_API window_submit_and_present(Window *p_window, gpu::CommandListHandle p_cmd) -> void;

	auto TST_KERNEL_API window_set_event_callback(Window *p_window, EventCallbackFn p_callback) -> void;
	auto TST_KERNEL_API window_set_event_callback_user_data(Window *p_window, void *p_user_data) -> void;

	auto TST_KERNEL_API window_is_resized(const Window *p_window) -> bool;
	auto TST_KERNEL_API window_is_maximised(const Window *p_window) -> bool;
	auto TST_KERNEL_API window_is_minimised(const Window *p_window) -> bool;
	auto TST_KERNEL_API window_is_fullscreen(const Window *p_window) -> bool;

	auto TST_KERNEL_API window_maximise(Window *p_window) -> void;
	auto TST_KERNEL_API window_minimise(Window *p_window) -> void;
	auto TST_KERNEL_API window_restore(Window *p_window) -> void;
	auto TST_KERNEL_API window_set_fullscreen(Window *p_window) -> void;
	auto TST_KERNEL_API window_set_windowed(Window *p_window) -> void;

	auto TST_KERNEL_API window_is_invalid_size(const Window *p_window) -> bool;
	auto TST_KERNEL_API window_get_size(const Window *p_window) -> tsm::uint2;
	auto TST_KERNEL_API window_get_aspect_ratio(const Window *p_window) -> float32;
}
