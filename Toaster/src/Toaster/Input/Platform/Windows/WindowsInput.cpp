#include "tstpch.h"
#include "WindowsInput.hpp"

#include "GLFW/glfw3.h"
#include "Toaster/Application.hpp"

#ifdef TST_PLATFORM_WINDOWS
namespace tst
{
	Input* Input::m_instance = new WindowsInput();


	bool WindowsInput::isKeyPressedPlatformNative(TstKeycode keycode)
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());
		TstKeyState state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}
	bool WindowsInput::isMouseButtonPressedPlatformNative(TstMouseButton button)
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());
		TstMouseButtonState state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	double WindowsInput::getMouseX_PlatformNative()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		return x;
	}

	double WindowsInput::getMouseY_PlatformNative()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		return y;
	}

	TstMousePos WindowsInput::getMousePosPlatformNative()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		return {x, y};
	}

}
#endif