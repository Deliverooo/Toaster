#pragma once
#include "Event.hpp"


namespace tst {

	class TST_API MouseMoveEvent : public Event {
	public:

		MouseMoveEvent(const float x, const float y) : m_mouseX(x), m_mouseY(y) {}

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse);
		EVENT_CLASS_TYPE(MouseMoved);

		float getMouseX() const { return m_mouseX; }
		float getMouseY() const { return m_mouseY; }

		std::string toStr() const override {

			return std::string("Mouse Move Event -> [" + std::to_string(m_mouseX) + "," + std::to_string(m_mouseY) + "]");
		}

	private:
		float m_mouseX;
		float m_mouseY;
	};

	class TST_API MouseScrollEvent : public Event {
	public:

		MouseScrollEvent(const float xOff, const float yOff) : m_scrollX(xOff), m_scrollY(yOff) {}

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse);
		EVENT_CLASS_TYPE(MouseScrolled);

		float getScrollX() const { return m_scrollX; }
		float getScrollY() const { return m_scrollY; }

		std::string toStr() const override {
			return std::string("Mouse Scroll Event -> [" + std::to_string(m_scrollX) + "," + std::to_string(m_scrollY) + "]");
		}

	private:
		float m_scrollX;
		float m_scrollY;
	};

	class TST_API MouseButtonEvent : public Event {
	public:

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse);

		int getMouseButton() const { return m_mouseButton; }

	protected:
		MouseButtonEvent(int button) : m_mouseButton(button){}

		int m_mouseButton;
	};

	class TST_API MouseButtonPressEvent : public MouseButtonEvent {
	public:

		MouseButtonPressEvent(int button) : MouseButtonEvent(button){}

		EVENT_CLASS_TYPE(MouseButtonPressed);

		std::string toStr() const override {

			return std::string("Mouse Button Pressed Event -> [" + std::to_string(m_mouseButton) + "]");
		}
	};

	class TST_API MouseButtonReleaseEvent : public MouseButtonEvent {
	public:

		MouseButtonReleaseEvent(int button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonReleased);

		std::string toStr() const override {

			return std::string("Mouse Button Released Event -> [" + std::to_string(m_mouseButton) + "]");
		}
	};

	class TST_API MouseButtonHeldEvent : public MouseButtonEvent {
	public:

		MouseButtonHeldEvent(int button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonHeld);

		std::string toStr() const override {

			return std::string("Mouse Button Held Event -> [" + std::to_string(m_mouseButton) + "]");
		}
	};
}