#pragma once
#include "tstpch.h"
#include "Event.hpp"

namespace tst {

	class TST_API WindowClosedEvent : public Event
	{
	public:

		WindowClosedEvent() = default;

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		EVENT_CLASS_TYPE(WindowClosed);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Window Closed Event -> [Window Closed]");
		}
	};

	class TST_API WindowResizedEvent : public Event {
	public:

		WindowResizedEvent(int width, int height) : m_width(width), m_height(height) {}

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		EVENT_CLASS_TYPE(WindowResized);

		[[nodiscard]] int getWidth() const {return m_width;}
		[[nodiscard]] int getHeight() const { return m_height; }

		[[nodiscard]] std::string toStr() const override {
			return std::string("Window Resized Event -> [" + std::to_string(m_width) + "," + std::to_string(m_height) + "]");
		}

	private:
		int m_width;
		int m_height;
	};

	class TST_API WindowMaximizedEvent : public Event {
	public:

		WindowMaximizedEvent(int maximized) : m_maximized(maximized){}

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		EVENT_CLASS_TYPE(WindowMaximized);

		[[nodiscard]] int getMaximized() const { return m_maximized; }

		[[nodiscard]] std::string toStr() const override {
			return std::string("Window Maximized Event -> [" + std::to_string(m_maximized) + "]");
		}

	private:
		int m_maximized;
	};

	class TST_API FrameBufferResizeEvent : public Event {
	public:

		FrameBufferResizeEvent(int width, int height) : m_width(width), m_height(height) {}

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		EVENT_CLASS_TYPE(FramebufferResized);

		[[nodiscard]] int getWidth() const { return m_width; }
		[[nodiscard]] int getHeight() const { return m_height; }

		[[nodiscard]] std::string toStr() const override {
			return std::string("Framebuffer Resize Event -> [" + std::to_string(m_width) + "," + std::to_string(m_height) + "]");
		}

	private:
		int m_width;
		int m_height;
	};

	class TST_API WindowFocusEvent : public Event
	{
	public:

		EVENT_CLASS_CATEGORY(EventCategoryApplication);
		[[nodiscard]] int getFocus() const { return m_focus; }

	protected:
		WindowFocusEvent(int focus) : m_focus(focus){}
		int m_focus;
	};

	class TST_API WindowLostFocusEvent : public WindowFocusEvent
	{
	public:
		WindowLostFocusEvent(int focus) : WindowFocusEvent(focus) {}

		EVENT_CLASS_TYPE(WindowLostFocus);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Window Focus Event -> [Lost Focus]");
		}
	};

	class TST_API WindowGainedFocusEvent : public WindowFocusEvent
	{
	public:
		WindowGainedFocusEvent(int focus) : WindowFocusEvent(focus) {}

		EVENT_CLASS_TYPE(WindowGainedFocus);

		[[nodiscard]] std::string toStr() const override {
			return std::string("Window Focus Event -> [Gained Focus]");
		}
	};
}