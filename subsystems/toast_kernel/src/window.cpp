#include "toast_kernel/window.hpp"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <dwmapi.h>
#include <GLFW/glfw3native.h>

#include "toast_gpu/frame.hpp"
#include "toast_kernel/events/window_event.hpp"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace toaster
{
	auto Window::initWindowingAPI() -> void
	{
		glfwInit();
	}

	auto Window::shutdownWindowingAPI() -> void
	{
		glfwTerminate();
	}

	Window::Window(const WindowDesc &p_desc)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		m_cbData.width  = p_desc.size.x;
		m_cbData.height = p_desc.size.y;
		m_window        = glfwCreateWindow(static_cast<int32>(m_cbData.width), static_cast<int32>(m_cbData.height), p_desc.title ? p_desc.title : "", nullptr, nullptr);
		TST_PERMA_ASSERT(m_window);

		m_cbData.inputCtx = InputContext{m_window};

		glfwSetWindowUserPointer(m_window, &m_cbData);

		glfwSetFramebufferSizeCallback(m_window, +[](GLFWwindow *p_window, int32 p_width, int32 p_height) -> void
		{
			auto data{static_cast<CallbackData *>(glfwGetWindowUserPointer(p_window))};
			data->resized = true;
			data->width   = static_cast<uint32>(p_width);
			data->height  = static_cast<uint32>(p_height);
		});

		glfwSetKeyCallback(m_window, +[](GLFWwindow *p_window, int32 p_key, int32 p_scancode, int32 p_action, int32 p_mods) -> void
		{
			auto data{static_cast<CallbackData *>(glfwGetWindowUserPointer(p_window))};

			if (p_action == GLFW_PRESS)
				data->inputCtx.m_currentKeyStates[static_cast<EKeyCode>(p_key)] = true;
			else if (p_action == GLFW_RELEASE)
				data->inputCtx.m_currentKeyStates[static_cast<EKeyCode>(p_key)] = false;
		});

		glfwSetMouseButtonCallback(m_window, +[](GLFWwindow *p_window, int32 p_button, int32 p_action, int32 p_mods) -> void
		{
			auto data{static_cast<CallbackData *>(glfwGetWindowUserPointer(p_window))};

			if (p_action == GLFW_PRESS)
				data->inputCtx.m_currentMouseStates[static_cast<EMouseButton>(p_button)] = true;
			else if (p_action == GLFW_RELEASE)
				data->inputCtx.m_currentMouseStates[static_cast<EMouseButton>(p_button)] = false;
		});

		glfwSetCursorPosCallback(m_window, +[](GLFWwindow *p_window, float64 p_xpos, float64 p_ypos) -> void
		{
			auto data{static_cast<CallbackData *>(glfwGetWindowUserPointer(p_window))};

			if (data->inputCtx.m_firstMouse)
			{
				data->inputCtx.m_mouseX     = static_cast<float32>(p_xpos);
				data->inputCtx.m_mouseY     = static_cast<float32>(p_ypos);
				data->inputCtx.m_firstMouse = false;
			}
			data->inputCtx.m_mouseDx = static_cast<float32>(p_xpos) - data->inputCtx.m_mouseX;
			data->inputCtx.m_mouseDy = data->inputCtx.m_mouseY - static_cast<float32>(p_ypos);

			data->inputCtx.m_mouseX = static_cast<float32>(p_xpos);
			data->inputCtx.m_mouseY = static_cast<float32>(p_ypos);
		});

		glfwSetScrollCallback(m_window, +[](GLFWwindow *p_window, float64 p_x_offset, float64 p_y_offset) -> void
		{
			auto data{static_cast<CallbackData *>(glfwGetWindowUserPointer(p_window))};

			data->inputCtx.m_scrollX = static_cast<float32>(p_x_offset);
			data->inputCtx.m_scrollY = static_cast<float32>(p_y_offset);
		});

		glfwSetCursorEnterCallback(m_window, +[](GLFWwindow *p_window, int32 p_entered) -> void
		{
			auto data{static_cast<CallbackData *>(glfwGetWindowUserPointer(p_window))};

			if (p_entered == GLFW_TRUE)
				data->inputCtx.m_firstMouse = true;
		});

		//TODO: Actually finish setting up the callbacks...

		HWND hwnd{glfwGetWin32Window(m_window)};

		// Dark titlebar looks good.
		bool32 use_dark_mode{true};
		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_mode, sizeof(bool32));

		// Sharp corners may be more aerodynamic on lower-end GPUs.
		uint32 corner_preference{DWMWCP_DONOTROUND};
		DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_preference, sizeof(uint32));

		m_windowSurface = gpu::createSurface(hwnd);
		m_swapchain     = gpu::createSwapchain(m_windowSurface, {m_cbData.width, m_cbData.height});

		if (p_desc.startMaximized)
			maximiseWindow();

		glfwShowWindow(m_window);
	}

	Window::~Window()
	{
		gpu::destroySwapchain(m_swapchain);
		gpu::destroySurface(m_windowSurface);

		glfwDestroyWindow(m_window);
	}

	auto Window::processMessages() const -> bool
	{
		glfwPollEvents();
		return !glfwWindowShouldClose(m_window);
	}

	auto Window::beginFrame(gpu::CommandListHandle p_cmd) -> bool
	{
		m_currentTexture = gpu::acquireNextImage(m_swapchain);

		if (!m_currentTexture)
		{
			gpu::resizeSwapchain(m_swapchain, getSize());
			return false;
		}

		gpu::insertPreRenderSwapchainResourceBarrier(p_cmd, m_currentTexture);

		return true;
	}

	auto Window::submitAndPresent(gpu::CommandListHandle p_cmd) -> void
	{
		gpu::frame::submitAndPresent(m_swapchain, p_cmd);

		// Set the current key states to the previous ones
		m_cbData.inputCtx.update();
	}

	auto Window::checkForResize() -> bool
	{
		if (!m_cbData.resized)
			return true;

		if (isInvalidSize())
			return false;

		if (!gpu::resizeSwapchain(m_swapchain, getSize()))
			return false;

		// Instead of dispatching the window resize from the GLFW callback, it is safer to do it here. As to not interfere with any Vulkan code.
		WindowResizeEvent event{getSize()};
		if (m_cbData.eventCallback)
			m_cbData.eventCallback(event);

		m_cbData.resized = false;
		return true;
	}

	auto Window::isFullscreen() const -> bool
	{
		return glfwGetWindowMonitor(m_window);
	}

	auto Window::maximiseWindow() -> void
	{
		glfwMaximizeWindow(m_window);
	}

	auto Window::minimiseWindow() -> void
	{
		glfwIconifyWindow(m_window);
	}

	auto Window::restoreWindow() -> void
	{
		glfwRestoreWindow(m_window);
	}

	auto Window::setFullscreen() -> void
	{
		GLFWmonitor *      monitor{glfwGetPrimaryMonitor()};
		const GLFWvidmode *mode{glfwGetVideoMode(monitor)};
		glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	auto Window::setWindowed() -> void
	{
		if (isFullscreen())
			glfwSetWindowMonitor(m_window, nullptr, 0, 0, static_cast<int32>(m_cbData.width), static_cast<int32>(m_cbData.height), 0);
	}
}
