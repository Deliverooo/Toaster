#pragma once

#include "input.hpp"
#include "events/event.hpp"
#include "toast_gpu/fwd.hpp"

struct GLFWwindow;

namespace toaster::rnd
{
	struct RenderContext;
}

namespace toaster::knl
{
	struct TST_KERNEL_API WindowDesc
	{
		math::UInt2 size;
		CString     title;
		bool        startMaximized;
	};

	enum class EWindowStateFlagBits : uint8
	{
		eResized = TST_LIB_BIT(0u), eMinimised = TST_LIB_BIT(1u), eMaximised = TST_LIB_BIT(2u)
	};

	TST_LIB_SPECIALISE_FLAGS(EWindowStateFlagBits, EWindowStateFlags);

	// The exact data that is passed as a user data pointer to the wnd proc
	struct TST_KERNEL_API _WindowCallbackData
	{
		InputContext inputCtx;

		EventCallbackFn eventCallback;
		void *          eventCallbackUserData;

		uint32 width;
		uint32 height;

		EWindowStateFlags stateFlags;
	};

	struct TST_KERNEL_API Window
	{
		GLFWwindow *window;

		_WindowCallbackData cbData;

		rnd::RenderContext *renderCtx; // Non-owning

		gpu::SurfaceHandle   surface;
		gpu::SwapchainHandle swapchain;

		gpu::TextureHandle currentTexture;
	};

	// These just initialise and shutdown GLFW...
	auto TST_KERNEL_API window_init_api() -> void;
	auto TST_KERNEL_API window_shutdown_api() -> void;

	auto TST_KERNEL_API window_create(Window *p_out_window, rnd::RenderContext *p_render_ctx, const WindowDesc &p_desc) -> void;
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
	auto TST_KERNEL_API window_get_size(const Window *p_window) -> math::UInt2;
	auto TST_KERNEL_API window_get_aspect_ratio(const Window *p_window) -> float32;
}
