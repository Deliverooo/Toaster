#include "tstpch.h"

#include "Toaster/Core/Input.hpp"

#include "GLFW/glfw3.h"
#include "Toaster/Core/Application.hpp"

#ifdef TST_PLATFORM_WINDOWS
namespace tst
{
	bool Input::isKeyPressed(TstKeycode keycode)
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());
		TstKeyState state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::isMouseButtonPressed(TstMouseButton button)
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());
		TstMouseButtonState state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}


	double Input::getMouseX()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		return x;
	}

	double Input::getMouseY()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		return y;
	}

	TstMousePos Input::getMousePos()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		double x, y;
		glfwGetCursorPos(window, &x, &y);

		return {x, y};
	}

	void Input::focusMouseCursor()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	void Input::unfocusMouseCursor()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	bool Input::isMouseCursorFocused()
	{
		Application& app = Application::getInstance();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.getWindow().getWindow());

		return (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED);
	}

}
#endif