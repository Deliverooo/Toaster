#include "toast_kernel/input.hpp"
#include "toast_kernel/window.hpp"

#include <GLFW/glfw3.h>

namespace toaster
{
	auto InputContext::isKeyDown(EKeyCode p_key_code) const -> bool
	{
		return m_currentKeyStates[static_cast<uint32>(p_key_code)];
	}

	auto InputContext::isKeyPressed(EKeyCode p_key_code) const -> bool
	{
		return m_currentKeyStates[static_cast<uint32>(p_key_code)] && !m_previousKeyStates[static_cast<uint32>(p_key_code)];
	}

	auto InputContext::isKeyReleased(EKeyCode p_key_code) const -> bool
	{
		return !m_currentKeyStates[static_cast<uint32>(p_key_code)] && m_previousKeyStates[static_cast<uint32>(p_key_code)];
	}

	auto InputContext::isMouseButtonDown(EMouseButton p_button) const -> bool
	{
		return m_currentMouseStates[static_cast<uint32>(p_button)];
	}

	auto InputContext::isMouseButtonPressed(EMouseButton p_button) const -> bool
	{
		return m_currentMouseStates[static_cast<uint32>(p_button)] && !m_previousMouseStates[static_cast<uint32>(p_button)];
	}

	auto InputContext::isMouseButtonReleased(EMouseButton p_button) const -> bool
	{
		return !m_currentMouseStates[static_cast<uint32>(p_button)] && m_previousMouseStates[static_cast<uint32>(p_button)];
	}

	auto InputContext::setCursorMode(ECursorMode p_mode) -> void
	{
		glfwSetInputMode(static_cast<GLFWwindow *>(m_windowCtx), GLFW_CURSOR, GLFW_CURSOR_NORMAL + static_cast<int32>(p_mode));
		m_currentCursorMode = p_mode;
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
		m_currentKeyStates.reset();
		m_previousKeyStates.reset();

		m_currentMouseStates.reset();
		m_previousMouseStates.reset();
	}
}
