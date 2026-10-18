#include "toast_kernel/window.hpp"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <dwmapi.h>
#include <GLFW/glfw3native.h>

#include "toast_gpu/frame.hpp"
#include "toast_kernel/events/key_event.hpp"
#include "toast_kernel/events/window_event.hpp"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace toaster::ke
{
	auto window_init_api() -> void
	{
		glfwInit();
	}

	auto window_shutdown_api() -> void
	{
		glfwTerminate();
	}

	auto window_create(Window *p_out_window, const WindowDesc &p_desc) -> void
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		p_out_window->cbData.width = p_desc.size.x;
		p_out_window->cbData.height = p_desc.size.y;
		p_out_window->window = glfwCreateWindow(static_cast<int32>(p_out_window->cbData.width), static_cast<int32>(p_out_window->cbData.height), p_desc.title, nullptr,
												nullptr);
		TST_PERMA_ASSERT(p_out_window->window);

		input_ctx_create(&p_out_window->cbData.inputCtx, p_out_window->window);

		glfwSetWindowUserPointer(p_out_window->window, &p_out_window->cbData);

		#define TST_DISPATCH_EVENT(__eventClass, ...) __eventClass event{__VA_ARGS__};\
				if(data->eventCallback)\
				data->eventCallback(event, data->eventCallbackUserData)

		#define TST_GET_CALLBACK_DATA() auto data{static_cast<Window::CallbackData *>(glfwGetWindowUserPointer(p_window))}

		glfwSetFramebufferSizeCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_width, int32 p_height) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();
			data->stateFlags |= EWindowStateFlagBits::eResized;
			data->width      = static_cast<uint32>(p_width);
			data->height     = static_cast<uint32>(p_height);
		});

		glfwSetKeyCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_key, int32 p_scancode, int32 p_action, int32 p_mods) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();

			if (p_key == GLFW_KEY_UNKNOWN)
				return;

			EKeyCode keycode{static_cast<EKeyCode>(p_key)};

			if (p_action == GLFW_PRESS)
			{
				data->inputCtx.currentKeyStates[static_cast<uint32>(keycode)] = true;
				TST_DISPATCH_EVENT(KeyPressEvent, keycode);
			}
			else if (p_action == GLFW_RELEASE)
			{
				data->inputCtx.currentKeyStates[static_cast<uint32>(keycode)] = false;
				TST_DISPATCH_EVENT(KeyReleaseEvent, keycode);
			}
		});

		glfwSetMouseButtonCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_button, int32 p_action, int32 p_mods) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();
			EMouseButton button{static_cast<EMouseButton>(p_button)};

			if (p_action == GLFW_PRESS)
			{
				data->inputCtx.currentMouseStates[static_cast<uint32>(button)] = true;
				// TST_DISPATCH_EVENT(MouseButtonPressEvent, button);
			}
			else if (p_action == GLFW_RELEASE)
			{
				data->inputCtx.currentMouseStates[static_cast<uint32>(button)] = false;
				// TST_DISPATCH_EVENT(MouseButtonReleaseEvent, button);
			}
		});

		glfwSetCursorPosCallback(p_out_window->window, +[](GLFWwindow *p_window, float64 p_xpos, float64 p_ypos) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();

			if (data->inputCtx.firstMouse)
			{
				data->inputCtx.mouseX     = static_cast<float32>(p_xpos);
				data->inputCtx.mouseY     = static_cast<float32>(p_ypos);
				data->inputCtx.firstMouse = false;
			}
			data->inputCtx.mouseDx = static_cast<float32>(p_xpos) - data->inputCtx.mouseX;
			data->inputCtx.mouseDy = data->inputCtx.mouseY - static_cast<float32>(p_ypos);

			data->inputCtx.mouseX = static_cast<float32>(p_xpos);
			data->inputCtx.mouseY = static_cast<float32>(p_ypos);
		});

		glfwSetScrollCallback(p_out_window->window, +[](GLFWwindow *p_window, float64 p_x_offset, float64 p_y_offset) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();

			data->inputCtx.scrollX = static_cast<float32>(p_x_offset);
			data->inputCtx.scrollY = static_cast<float32>(p_y_offset);
		});

		glfwSetCursorEnterCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_entered) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();

			if (p_entered == GLFW_TRUE)
				data->inputCtx.firstMouse = true;
		});

		glfwSetWindowFocusCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_focused) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();

			if (!p_focused) // Clear input states if the window has lost focus
			{
				input_ctx_clear_states(&data->inputCtx);
				data->inputCtx.firstMouse = true;
			}
		});

		glfwSetDropCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_path_count, CString p_paths[]) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();

			std::vector<String> filepaths{static_cast<std::vector<String>::size_type>(p_path_count)};
			for (uint32 i{0u}; i < p_path_count; ++i)
				filepaths[i] = p_paths[i];

			TST_DISPATCH_EVENT(WindowFileDropEvent, filepaths);
		});

		glfwSetWindowMaximizeCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_maximised) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();
			data->stateFlags = (p_maximised) ? (data->stateFlags | EWindowStateFlagBits::eMaximised) : (data->stateFlags & ~EWindowStateFlagBits::eMaximised);
			data->stateFlags &= ~EWindowStateFlagBits::eMinimised;
		});

		glfwSetWindowIconifyCallback(p_out_window->window, +[](GLFWwindow *p_window, int32 p_iconified) noexcept -> void
		{
			TST_GET_CALLBACK_DATA();
			data->stateFlags = (p_iconified) ? (data->stateFlags | EWindowStateFlagBits::eMinimised) : (data->stateFlags & ~EWindowStateFlagBits::eMinimised);
			data->stateFlags &= ~EWindowStateFlagBits::eMaximised;
		});

		#undef TST_GET_CALLBACK_DATA
		#undef TST_DISPATCH_EVENT

		HWND hwnd{glfwGetWin32Window(p_out_window->window)};

		// Dark titlebar looks good.
		bool32 use_dark_mode{true};
		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_mode, sizeof(bool32));

		// Sharp corners may be more aerodynamic on lower-end GPUs.
		uint32 corner_preference{DWMWCP_DONOTROUND};
		DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_preference, sizeof(uint32));

		p_out_window->surface   = gpu::createSurface(hwnd);
		p_out_window->swapchain = gpu::createSwapchain(p_out_window->surface, {p_out_window->cbData.width, p_out_window->cbData.height});

		if (p_desc.startMaximized)
			window_maximise(p_out_window);

		glfwShowWindow(p_out_window->window);
	}

	auto window_destroy(Window *p_window) -> void
	{
		gpu::destroySwapchain(p_window->swapchain);
		gpu::destroySurface(p_window->surface);

		input_ctx_destroy(&p_window->cbData.inputCtx);
		glfwDestroyWindow(p_window->window);

		p_window->window         = nullptr;
		p_window->swapchain      = nullptr;
		p_window->surface        = nullptr;
		p_window->currentTexture = nullptr;
	}

	auto window_process_messages(const Window *p_window) -> bool
	{
		glfwPollEvents();
		return !glfwWindowShouldClose(p_window->window);
	}

	auto window_check_for_resize(Window *p_window) -> bool
	{
		if (!(p_window->cbData.stateFlags & EWindowStateFlagBits::eResized))
			return true;

		if (window_is_invalid_size(p_window))
			return false;

		tsm::uint2 current_size{p_window->cbData.width, p_window->cbData.height};

		if (!gpu::resizeSwapchain(p_window->swapchain, current_size))
		{
			WindowResizeEvent event{current_size};
			if (p_window->cbData.eventCallback)
				p_window->cbData.eventCallback(event, p_window->cbData.eventCallbackUserData);
			return false;
		}

		// Instead of dispatching the window resize from the GLFW callback, it is safer to do it here. As to not interfere with any Vulkan code.
		WindowResizeEvent event{current_size};
		if (p_window->cbData.eventCallback)
			p_window->cbData.eventCallback(event, p_window->cbData.eventCallbackUserData);

		p_window->cbData.stateFlags &= ~EWindowStateFlagBits::eResized;
		return true;
	}

	auto window_begin_frame(Window *p_window, gpu::CommandListHandle p_cmd) -> bool
	{
		p_window->currentTexture = gpu::acquireNextImage(p_window->swapchain);

		if (!p_window->currentTexture)
		{
			const tsm::uint2 current_size{window_get_size(p_window)};

			gpu::resizeSwapchain(p_window->swapchain, current_size);

			WindowResizeEvent event{current_size};
			if (p_window->cbData.eventCallback)
				p_window->cbData.eventCallback(event, p_window->cbData.eventCallbackUserData);

			return false;
		}

		gpu::insertPreRenderSwapchainResourceBarrier(p_cmd, p_window->currentTexture);

		return true;
	}

	auto window_submit_and_present(Window *p_window, gpu::CommandListHandle p_cmd) -> void
	{
		const bool success{gpu::frame::submitAndPresent(p_window->swapchain, p_cmd)};
		TST_PERMA_ASSERT(success);

		// Set the current key states to the previous ones
		input_ctx_update(&p_window->cbData.inputCtx);
	}

	auto window_set_event_callback(Window *p_window, EventCallbackFn p_callback) -> void
	{
		p_window->cbData.eventCallback = p_callback;
	}

	auto window_set_event_callback_user_data(Window *p_window, void *p_user_data) -> void
	{
		p_window->cbData.eventCallbackUserData = p_user_data;
	}

	auto window_is_resized(const Window *p_window) -> bool
	{
		return !!(p_window->cbData.stateFlags & EWindowStateFlagBits::eResized);
	}

	auto window_is_maximised(const Window *p_window) -> bool
	{
		return !!(p_window->cbData.stateFlags & EWindowStateFlagBits::eMaximised);
	}

	auto window_is_minimised(const Window *p_window) -> bool
	{
		return !!(p_window->cbData.stateFlags & EWindowStateFlagBits::eMinimised);
	}

	auto window_is_fullscreen(const Window *p_window) -> bool
	{
		return glfwGetWindowMonitor(p_window->window);
	}

	auto window_maximise(Window *p_window) -> void
	{
		glfwMaximizeWindow(p_window->window);
	}

	auto window_minimise(Window *p_window) -> void
	{
		glfwIconifyWindow(p_window->window);
	}

	auto window_restore(Window *p_window) -> void
	{
		glfwRestoreWindow(p_window->window);
	}

	auto window_set_fullscreen(Window *p_window) -> void
	{
		GLFWmonitor *      monitor{glfwGetPrimaryMonitor()};
		const GLFWvidmode *mode{glfwGetVideoMode(monitor)};
		glfwSetWindowMonitor(p_window->window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	auto window_set_windowed(Window *p_window) -> void
	{
		if (window_is_fullscreen(p_window))
			glfwSetWindowMonitor(p_window->window, nullptr, 0, 0, static_cast<int32>(p_window->cbData.width), static_cast<int32>(p_window->cbData.height), 0);
	}

	auto window_is_invalid_size(const Window *p_window) -> bool
	{
		return p_window->cbData.width == 0u || p_window->cbData.height == 0u;
	}

	auto window_get_size(const Window *p_window) -> tsm::uint2
	{
		return tsm::uint2{p_window->cbData.width, p_window->cbData.height};
	}

	auto window_get_aspect_ratio(const Window *p_window) -> float32
	{
		const tsm::uint2 size{window_get_size(p_window)};
		return size.aspect();
	}
}
