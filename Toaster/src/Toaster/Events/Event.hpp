#pragma once  

#include "Toaster/Core.hpp"
#include <functional>
#include <string>

namespace tst {  

	enum class EventType  
	{  
		None = 0,  
		WindowClosed, WindowResized, WindowMinimized, WindowMoved,  
		KeyPressed, KeyReleased, KeyHeld,  
		MouseButtonPressed, MouseButtonReleased, MouseButtonHeld, MouseMoved, MouseScrolled,  
	};  

	enum EventCategory  
	{  

		// the event categories are represented as a set of bits
		// e.g. an event of type "EventCategoryApplication" would have the bits 0001
		// as is the case with bits, you can also use the | or operator to combine them together
		None = 0,  
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
	};  

#define EVENT_CLASS_TYPE(type) static EventType getStaticType() {return EventType::##type;}\
							   EventType getEventType() const override {return getStaticType();}\
							   const char* getEventName() const override {return #type;}

#define EVENT_CLASS_CATEGORY(category) virtual int getEventCategory() const override {return category;}

	// an abstract class defining the outline of what an event should have
	class TST_API Event {  
	public:

		virtual EventType getEventType() const = 0;  
		virtual const char* getEventName() const = 0;  
		virtual int getEventCategory() const = 0;
		virtual std::string toStr() const = 0;

		bool inCategory(EventCategory category) {
			return getEventCategory() & category;
		}

	protected:  
		bool m_isHandled = false;
	};

	std::string format_as(const Event& e) {
		return e.toStr();
	}

	template<typename T>
	using EventFunc = std::function<bool(T&)>;

	class TST_API EventDispatcher {
	public:
		EventDispatcher(Event& event) : m_event(event) {}

		template<typename T>
		bool dispatch(EventFunc<T> func) {

			if (m_event.getEventType == T::getStaticType()) {
				m_event.m_isHandled = func(m_event);
				return true;
			}

			return false;
		}

	private:
		Event& m_event;
	};
}
