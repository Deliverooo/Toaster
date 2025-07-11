#pragma once
#include "Event.hpp"

namespace tst {

	class TST_API MouseMoveEvent : public Event {
	public:

		MouseMoveEvent(const double x, const double y) : m_mouseX(x), m_mouseY(y) {}

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse);
		EVENT_CLASS_TYPE(MouseMoved);

		[[nodiscard]] double getMouseX() const { return m_mouseX; }
		[[nodiscard]] double getMouseY() const { return m_mouseY; }

		[[nodiscard]] std::string toStr() const override {
			return std::string("Mouse Move Event -> [" + std::to_string(m_mouseX) + "," + std::to_string(m_mouseY) + "]");
		}

	private:
		double m_mouseX;
		double m_mouseY;
	};

	class TST_API MouseScrollEvent : public Event {
	public:

		MouseScrollEvent(const double xOff, const double yOff) : m_scrollX(xOff), m_scrollY(yOff) {}

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse);
		EVENT_CLASS_TYPE(MouseScrolled);

		[[nodiscard]] double getScrollX() const { return m_scrollX; }
		[[nodiscard]] double getScrollY() const { return m_scrollY; }

		[[nodiscard]] std::string toStr() const override {
			return std::string("Mouse Scroll Event -> [" + std::to_string(m_scrollX) + "," + std::to_string(m_scrollY) + "]");
		}

	private:
		double m_scrollX;
		double m_scrollY;
	};

	class TST_API MouseButtonEvent : public Event {
	public:

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse);

		[[nodiscard]] int getMouseButton() const { return m_mouseButton; }

	protected:
		MouseButtonEvent(int button) : m_mouseButton(button){}

		int m_mouseButton;
	};

	class TST_API MouseButtonPressEvent : public MouseButtonEvent {
	public:

		MouseButtonPressEvent(int button) : MouseButtonEvent(button){}

		EVENT_CLASS_TYPE(MouseButtonPressed);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Mouse Button Pressed Event -> [" + std::to_string(m_mouseButton) + "]");
		}
	};

	class TST_API MouseButtonReleaseEvent : public MouseButtonEvent {
	public:

		MouseButtonReleaseEvent(int button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonReleased);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Mouse Button Released Event -> [" + std::to_string(m_mouseButton) + "]");
		}
	};

	class TST_API MouseButtonHeldEvent : public MouseButtonEvent {
	public:

		MouseButtonHeldEvent(int button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonHeld);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Mouse Button Held Event -> [" + std::to_string(m_mouseButton) + "]");
		}
	};
}