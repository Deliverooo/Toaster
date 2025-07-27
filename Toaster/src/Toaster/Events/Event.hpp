#pragma once

#include "tstpch.h"
#include "Toaster/Core/Core.hpp"


namespace tst {  

	enum class EventType  
	{  
		None = 0,  
		WindowClosed, WindowResized, WindowLostFocus, WindowGainedFocus, WindowMoved, WindowMaximized,
		KeyPressed, KeyReleased, KeyHeld, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseButtonHeld, MouseMoved, MouseScrolled,
		FramebufferResized,
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

		virtual ~Event(){}

		[[nodiscard]] virtual EventType getEventType() const = 0;  
		[[nodiscard]] virtual const char* getEventName() const = 0;  
		[[nodiscard]] virtual int getEventCategory() const = 0;
		[[nodiscard]] virtual std::string toStr() const = 0;

		[[nodiscard]] bool inCategory(const EventCategory category) const {
			return getEventCategory() & category;
		}

		[[nodiscard]] bool isHandled() { return m_isHandled; }

		bool m_isHandled = false;
	protected:  

		friend class EventDispatcher;
	};

	inline std::string format_as(const Event &event)
	{
		return event.toStr();
	}

	template<typename T>
	using EventFunc = std::function<bool(T&)>;

	// EventDispatcher is a utility class that allows for easy dispatching of events
	// It takes an event and a function, and calls the function if the event type matches
	class TST_API EventDispatcher {
	public:
		// Constructor that takes an event reference
		EventDispatcher(Event& event) : m_event(event) {}

		// Dispatch function that takes a function and calls it if the event type matches
		template<typename T>
		bool dispatch(EventFunc<T> func) {

			TST_ASSERT((std::derived_from<T, Event>), "Event Dispatcher expected Event Type!");
			
			// Check if the event type matches the type T
			if (m_event.getEventType() == T::getStaticType()) {
				// Cast the event to type T and call the function with it
				m_event.m_isHandled = func(static_cast<T&>(m_event));
				return true;
			}

			return false;
		}

	private:
		Event& m_event;
	};
}
