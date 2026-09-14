#include "toast_kernel/input.hpp"
#include "toast_kernel/window.hpp"

#include <GLFW/glfw3.h>

namespace toaster
{
	auto InputContext::isKeyDown(EKeyCode p_key_code) const -> bool
	{
		const auto it{m_currentKeyStates.find(p_key_code)};
		return it != m_currentKeyStates.end() && it->second;
	}

	auto InputContext::isKeyPressed(EKeyCode p_key_code) const -> bool
	{
		const auto current_it{m_currentKeyStates.find(p_key_code)};
		const auto previous_it{m_previousKeyStates.find(p_key_code)};

		const bool current_down{(current_it != m_currentKeyStates.end() && current_it->second)};
		const bool previous_down{(previous_it != m_previousKeyStates.end() && previous_it->second)};

		return current_down && !previous_down;
	}

	auto InputContext::isKeyReleased(EKeyCode p_key_code) const -> bool
	{
		const auto current_it{m_currentKeyStates.find(p_key_code)};
		const auto previous_it{m_previousKeyStates.find(p_key_code)};

		const bool current_down{(current_it != m_currentKeyStates.end() && current_it->second)};
		const bool previous_down{(previous_it != m_previousKeyStates.end() && previous_it->second)};

		return !current_down && previous_down;
	}

	auto InputContext::isMouseButtonDown(EMouseButton p_button) const -> bool
	{
		const auto it{m_currentMouseStates.find(p_button)};
		return it != m_currentMouseStates.end() && it->second;
	}

	auto InputContext::isMouseButtonPressed(EMouseButton p_button) const -> bool
	{
		const auto current_it{m_currentMouseStates.find(p_button)};
		const auto previous_it{m_previousMouseStates.find(p_button)};

		const bool current_down{(current_it != m_currentMouseStates.end() && current_it->second)};
		const bool previous_down{(previous_it != m_previousMouseStates.end() && previous_it->second)};

		return current_down && !previous_down;
	}

	auto InputContext::isMouseButtonReleased(EMouseButton p_button) const -> bool
	{
		const auto current_it{m_currentMouseStates.find(p_button)};
		const auto previous_it{m_previousMouseStates.find(p_button)};

		const bool current_down{(current_it != m_currentMouseStates.end() && current_it->second)};
		const bool previous_down{(previous_it != m_previousMouseStates.end() && previous_it->second)};

		return !current_down && previous_down;
	}

	auto InputContext::setCursorMode(ECursorMode p_mode) -> void
	{
		glfwSetInputMode(static_cast<GLFWwindow *>(m_windowCtx), GLFW_CURSOR, GLFW_CURSOR_NORMAL + static_cast<int32>(p_mode));
		m_cursorMode = p_mode;
	}

	auto InputContext::update() -> void
	{
		m_previousKeyStates   = m_currentKeyStates;
		m_previousMouseStates = m_currentMouseStates;

		m_mouseDx = 0.0f;
		m_mouseDy = 0.0f;

		m_scrollX = 0.0f;
		m_scrollY = 0.0f;
	}

	auto InputContext::clearStates() -> void
	{
		m_currentKeyStates.clear();
		m_previousKeyStates.clear();

		m_currentMouseStates.clear();
		m_previousMouseStates.clear();
	}
}
