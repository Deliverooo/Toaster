#include "tstpch.h"
#include "WindowsWindow.hpp"

#include <stb_image.h>

#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Events/KeyEvent.hpp"
#include "Toaster/Events/MouseEvent.hpp"


#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "dwmapi.h"

#define TST_WIN_DATA_FN(window) WindowData& window_data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window))

namespace tst
{

	Window* Window::Create(const WindowAttribArray& window_attribs)
	{
		return new WindowsWindow(window_attribs);
	}

	void WindowsWindow::shutdown()
	{
		TST_PROFILE_FN();

		assert(m_window != nullptr && "Found You!");
		TST_CORE_INFO("Destroyed Window!");

		glfwDestroyWindow(m_window);
		glfwTerminate();
		TST_CORE_INFO("Terminated GLFW!");

		m_window = nullptr;

		delete m_renderingContext;
	}


	WindowsWindow::~WindowsWindow() 
	{
		TST_PROFILE_FN();
		shutdown();
	}

	GLFWimage WindowsWindow::loadIcon(const char* filepath)
	{
		int width;
		int height;
		int nrChannels;

		unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

		if (!data)
		{
			TST_CORE_ERROR("Failed to load Window Icon! -> {0}", filepath);

			unsigned char fallbackData[3];
			fallbackData[0] = static_cast<unsigned char>(0xff);
			fallbackData[0] = static_cast<unsigned char>(0x00);
			fallbackData[0] = static_cast<unsigned char>(0xdc);

			GLFWimage fallbackIcon = { 1, 1, fallbackData };

			return fallbackIcon;
		}

		GLFWimage icon = { width, height, data };
		return icon;
	}



	WindowsWindow::WindowsWindow(const WindowAttribArray& window_attributes)
	{

		TST_PROFILE_FN();

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
		glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

		m_window = glfwCreateWindow(m_windowData.width, m_windowData.height, m_windowData.title, nullptr, nullptr);

		HWND hwnd = glfwGetWin32Window(m_window);

		COLORREF titleBarColour = RGB(0x25, 0x25, 0x25);
		//COLORREF textColour		= RGB(0x5f, 0x5f, 0x5f);
	
		DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleBarColour, sizeof(titleBarColour));
		//DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR,	 &textColour, sizeof(textColour));

		GLFWimage windowIcon = loadIcon(TST_CORE_RESOURCE_DIR"/images/OrboCloseup.png");
		glfwSetWindowIcon(m_window, 1, &windowIcon);

		glfwGetWindowPos(m_window, &m_windowPos.first, &m_windowPos.second);

		m_renderingContext = new OpenGLRenderingContext(m_window);
		m_renderingContext->init();


		glfwSwapInterval(m_windowData.vSyncEnabled);

		TST_CORE_INFO("V-Sync Enabled: {0}", m_windowData.vSyncEnabled);
		glfwSetWindowUserPointer(m_window, &m_windowData);

		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

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
				KeyPressedEvent keyPressedEvent(key, 0);
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

	std::pair<int, int> WindowsWindow::getPosition() const
	{
		return m_windowPos;
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

	float WindowsWindow::getAspect() const
	{
		return static_cast<float>(getWidth()) / static_cast<float>(getHeight());
	}

	void WindowsWindow::update()
	{
		TST_PROFILE_FN();

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
