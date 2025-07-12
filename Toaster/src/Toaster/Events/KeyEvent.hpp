#pragma once
#include "Event.hpp"

namespace tst {

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
			return std::string("Key Pressed Event -> [" + std::to_string(m_keyCode) + "]");
		}
	};

	class TST_API KeyTypedEvent : public KeyEvent {
	public:

		KeyTypedEvent(int keycode) : KeyEvent(keycode) {}

		EVENT_CLASS_TYPE(KeyTyped);


		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Typed Event -> [" + std::to_string(m_keyCode) + "]");
		}
	};

	class TST_API KeyReleasedEvent: public KeyEvent{
	public:

		KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}
		
		EVENT_CLASS_TYPE(KeyReleased);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Released Event -> [" + std::to_string(m_keyCode) + "]");
		}
	};

	class TST_API KeyHeldEvent : public KeyEvent {
	public:

		KeyHeldEvent(int keycode) : KeyEvent(keycode) {}

		EVENT_CLASS_TYPE(KeyHeld);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Key Held Event -> [" + std::to_string(m_keyCode) + "]\t[Held]");
		}

	};
}