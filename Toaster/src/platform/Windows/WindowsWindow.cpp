#include "tstpch.h"
#include "WindowsWindow.hpp"

#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Events/KeyEvent.hpp"
#include "Toaster/Events/MouseEvent.hpp"


#define TST_WIN_DATA_FN(window) WindowData& window_data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window))

namespace tst
{

	Window* Window::Create(const WindowAttribArray& window_attribs)
	{
		return new WindowsWindow(window_attribs);
	}

	WindowsWindow::~WindowsWindow() 
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}


	WindowsWindow::WindowsWindow(const WindowAttribArray& window_attributes)
	{

		m_windowData.width = window_attributes.width;
		m_windowData.height = window_attributes.height;
		m_windowData.title = window_attributes.title;
		m_windowData.vSyncEnabled = window_attributes.vSyncEnabled;

		
		TST_CORE_INFO("Creating Window... '{0}' -> [{1}, {2}]", m_windowData.title, m_windowData.width, m_windowData.height);

		TstResult success = glfwInit();
		if (!success)
		{
			TST_ERROR("[FATAL ERROR] -> FAILED TO INITIALIZE GLFW API!!!");
			return;
		}
		
		m_window = glfwCreateWindow(
			static_cast<int>(m_windowData.width),
			static_cast<int>(m_windowData.height),
			m_windowData.title,
			nullptr, nullptr
		);

		m_renderingContext = new OpenGLRenderingContext(m_window);
		m_renderingContext->init();

		glfwSwapInterval(m_windowData.vSyncEnabled);

		TST_CORE_INFO("V-Sync Enabled: {0}", m_windowData.vSyncEnabled);
		glfwSetWindowUserPointer(m_window, &m_windowData);

		glfwSetErrorCallback([](int errorCode, const char* description){

			TST_CORE_ERROR("GLFW ERROR {0} -> {1}", errorCode, description);
		});

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height){

			TST_WIN_DATA_FN(window);

			window_data.width = width;
			window_data.height = height;

			WindowResizedEvent windowResizedEvent(width, height);

			window_data.eventCallback(windowResizedEvent);
		});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window){

			TST_WIN_DATA_FN(window);
			WindowClosedEvent windowClosedEvent;
			window_data.eventCallback(windowClosedEvent);
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos){

			TST_WIN_DATA_FN(window);
			MouseMoveEvent mouseMoveEvent(xpos, ypos);
			window_data.eventCallback(mouseMoveEvent);
		});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods){

			TST_WIN_DATA_FN(window);

			switch (action)
			{
			case GLFW_PRESS: {
				MouseButtonPressEvent mouseButtonPressEvent(button);
				window_data.eventCallback(mouseButtonPressEvent);
				break;
			}

			case GLFW_RELEASE: {
				MouseButtonReleaseEvent mouseButtonReleaseEvent(button);
				window_data.eventCallback(mouseButtonReleaseEvent);
				break;
			}
			}

		});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {

			TST_WIN_DATA_FN(window);

			MouseScrollEvent mouseScrollEvent(xoffset, yoffset);
			window_data.eventCallback(mouseScrollEvent);
		});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {

			TST_WIN_DATA_FN(window);

			switch (action)
			{
			case GLFW_PRESS:
				{
				KeyPressedEvent keyPressedEvent(key);
				window_data.eventCallback(keyPressedEvent);
				break;
				}
			case GLFW_RELEASE:
				{
				KeyReleasedEvent keyReleasedEvent(key);
				window_data.eventCallback(keyReleasedEvent);
				break;
				}
			case GLFW_REPEAT:
				{
				KeyHeldEvent keyHeldEvent(key);
				window_data.eventCallback(keyHeldEvent);
				break;
				}
			}
		});

		glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode) {

			TST_WIN_DATA_FN(window);
			KeyTypedEvent keyTypedEvent(static_cast<unsigned short>(keycode));
			window_data.eventCallback(keyTypedEvent);
		});

		glfwSetWindowFocusCallback(m_window, [](GLFWwindow* window, int focused) {

			TST_WIN_DATA_FN(window);

			if (focused)
			{
				WindowGainedFocusEvent windowGainedFocusEvent(focused);
				window_data.eventCallback(windowGainedFocusEvent);
			}
			else
			{
				WindowLostFocusEvent windowLostFocusEvent(focused);
				window_data.eventCallback(windowLostFocusEvent);
			}
		});

		glfwSetWindowMaximizeCallback(m_window, [](GLFWwindow* window, int maximized){

			TST_WIN_DATA_FN(window);
			WindowMaximizedEvent windowMaximizedEvent(maximized);
			window_data.eventCallback(windowMaximizedEvent);
		});

		glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height){

			TST_WIN_DATA_FN(window);
			FrameBufferResizeEvent frameBufferResizeEvent(width, height);
			window_data.eventCallback(frameBufferResizeEvent);
		});
	}

	unsigned int WindowsWindow::getWidth() const
	{
		return m_windowData.width;
	}

	unsigned int WindowsWindow::getHeight() const
	{
		return m_windowData.height;
	}

	const char* WindowsWindow::getTitle() const
	{
		return m_windowData.title;
	}

	void WindowsWindow::update()
	{
		glfwPollEvents();
		m_renderingContext->swapBuffers();
	}

	void WindowsWindow::enableVsync(const bool yn)
	{
		glfwSwapInterval(yn);
	}

	bool WindowsWindow::getVsyncEnabled() const
	{
		return m_windowData.vSyncEnabled;
	}


}
