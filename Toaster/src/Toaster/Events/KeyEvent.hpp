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

		KeyPressedEvent(int keycode, bool repeating = false) : KeyEvent(keycode), m_repeating(repeating) {}

		EVENT_CLASS_TYPE(KeyPressed);

		[[nodiscard]] bool getIsRepeating() const { return m_repeating; }

		[[nodiscard]] std::string toStr() const override {

			return std::string("Key Pressed Event -> [" + std::to_string(m_keyCode) + "]\t[Repeating: " + std::to_string(m_repeating) + "]");
		}

	private:
		bool m_repeating;
	};

	class TST_API KeyReleasedEvent: public KeyEvent{
	public:

		KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}
		
		EVENT_CLASS_TYPE(KeyReleased);

		[[nodiscard]] std::string toStr() const override {

			return std::string("Key Released Event -> [" + std::to_string(m_keyCode) + "]");
		}
	private:
	};
}