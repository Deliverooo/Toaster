#pragma once

#include "Event.hpp"

namespace tst {

	class TST_API WindowClosedEvent : public Event {

	public:

		WindowClosedEvent(){}

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		EVENT_CLASS_TYPE(WindowClosed);

		std::string toStr() const override {
			return std::string("Window Closed Event -> ");
		}
	};

	class TST_API WindowResizedEvent : public Event {
	public:

		WindowResizedEvent(int width, int height) : m_width(width), m_height(height) {}

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		EVENT_CLASS_TYPE(WindowResized);

		int getWidth() const {return m_width;}
		int getHeight() const { return m_height; }

		std::string toStr() const override {
			return std::string("Window Resized Event -> [" + std::to_string(m_width) + "," + std::to_string(m_height) + "]");
			//return std::string("Orbo");
		}

	private:
		int m_width;
		int m_height;
	};
}