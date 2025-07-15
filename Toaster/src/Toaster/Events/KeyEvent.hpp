#pragma once
#include "Event.hpp"
#include "Toaster/KeyCodes.hpp"

namespace tst {

	inline std::string tstKeyCodeToString(int keycode)
	{
		switch (keycode)
		{
		case TST_KEY_UP:			return std::string("Up Arrow");
		case TST_KEY_DOWN:			return std::string("Down Arrow");
		case TST_KEY_LEFT:			return std::string("Left Arrow");
		case TST_KEY_RIGHT:			return std::string("Right Arrow");
		case TST_KEY_BACKSPACE:		return std::string("BackSpace");
		case TST_KEY_TAB:			return std::string("Tab");
		case TST_KEY_DELETE:		return std::string("Delete");
		case TST_KEY_SPACE:			return std::string("Space");
		case TST_KEY_ENTER:			return std::string("Enter");
		case TST_KEY_LEFT_SHIFT:	return std::string("Left Shift");
		case TST_KEY_RIGHT_SHIFT:	return std::string("Right Shift");
		case TST_KEY_LEFT_ALT:		return std::string("Left Alt");
		case TST_KEY_RIGHT_ALT:		return std::string("Right Alt");

#ifdef TST_PLATFORM_WINDOWS
		case TST_KEY_LEFT_SUPER:	return std::string("Left Win");
		case TST_KEY_RIGHT_SUPER:	return std::string("Right Win");
#else
		case TST_KEY_LEFT_SUPER:	return std::string("Left Cmd");
		case TST_KEY_RIGHT_SUPER:	return std::string("Right Cmd");
#endif

		case TST_KEY_CAPS_LOCK:		return std::string("CAPS LOCK");
		case TST_KEY_LEFT_CONTROL:	return std::string("Left Ctrl");
		case TST_KEY_RIGHT_CONTROL: return std::string("Right Ctrl");
		case TST_KEY_ESCAPE:		return std::string("Escape");
		case TST_KEY_PAGE_UP:		return std::string("Page Up");
		case TST_KEY_PAGE_DOWN:		return std::string("Page Down");
		case TST_KEY_HOME:			return std::string("Home");
		case TST_KEY_INSERT:		return std::string("Insert");
		case TST_KEY_END:			return std::string("End");
		case TST_KEY_PRINT_SCREEN:	return std::string("Print Screen");
		case TST_KEY_SCROLL_LOCK:	return std::string("Scroll Lock");
		case TST_KEY_PAUSE:			return std::string("Pause");

		default:
			return std::string(1, static_cast<char>(keycode));
		}
	}

	class TST_API KeyEvent : public Event {
	public:

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard);

		[[nodiscard]] int getKeycode() const { return m_keyCode; }

	protected:
		KeyEvent(int keycode) : m_keyCode(keycode){}

		int m_keyCode;
	};

	class TST_API KeyPressedEvent : public KeyEvent {
	public:

		KeyPressedEvent(int keycode) : KeyEvent(keycode) {}

		EVENT_CLASS_TYPE(KeyPressed);


		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Pressed Event -> [" + tstKeyCodeToString(m_keyCode) + "]");
		}
	};

	class TST_API KeyTypedEvent : public KeyEvent {
	public:

		KeyTypedEvent(int keycode) : KeyEvent(keycode) {}

		EVENT_CLASS_TYPE(KeyTyped);


		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Typed Event -> [" + tstKeyCodeToString(m_keyCode) + "]");
		}
	};

	class TST_API KeyReleasedEvent: public KeyEvent{
	public:

		KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}
		
		EVENT_CLASS_TYPE(KeyReleased);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Released Event -> [" + tstKeyCodeToString(m_keyCode) + "]");
		}
	};

	class TST_API KeyHeldEvent : public KeyEvent {
	public:

		KeyHeldEvent(int keycode) : KeyEvent(keycode) {}

		EVENT_CLASS_TYPE(KeyHeld);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Held Event -> [" + tstKeyCodeToString(m_keyCode) + "]\t[Held]");
		}

	};
}
