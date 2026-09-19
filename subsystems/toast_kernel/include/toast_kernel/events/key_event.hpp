#pragma once

#include "event.hpp"
#include "toast_kernel/input.hpp"

namespace toaster
{
	class TST_KERNEL_API KeyPressEvent final: public Event
	{
	public:
		KeyPressEvent(EKeyCode p_key_code) : m_keyCode(p_key_code)
		{
		}

		EVENT_CLASS_TYPE(eKeyPressed)

		auto getKeyCode() const -> EKeyCode { return m_keyCode; }

	private:
		EKeyCode m_keyCode;
	};

	class TST_KERNEL_API KeyReleaseEvent final: public Event
	{
	public:
		KeyReleaseEvent(EKeyCode p_key_code) : m_keyCode(p_key_code)
		{
		}

		EVENT_CLASS_TYPE(eKeyReleased)

		auto getKeyCode() const -> EKeyCode { return m_keyCode; }

	private:
		EKeyCode m_keyCode;
	};
}
