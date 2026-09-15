#pragma once

#include "../toast_kernel.hpp"

#include <functional>

namespace toaster
{
	enum class EEventType : uint8
	{
		eNone = 0u,
		eWindowClose,
		eWindowMinimize,
		eWindowMaximize,
		eWindowResize,
		eWindowFocus,
		eWindowLostFocus,
		eWindowMoved,
		eWindowFileDrop
	};

	enum EventCategory : uint8
	{
		None                      = 0u,
		EventCategory_Application = TST_BIT(0u),
		EventCategory_Input       = TST_BIT(1u),
		EventCategory_Keyboard    = TST_BIT(2u),
		EventCategory_Mouse       = TST_BIT(3u),
		EventCategory_MouseButton = TST_BIT(4u)
	};

	#define EVENT_CLASS_TYPE(__type) static auto getStaticType() -> EEventType {return EEventType::__type;}\
							   auto getEventType() const -> EEventType override {return getStaticType();}\
							   auto getEventName() const -> CString override {return #__type;}

	#define EVENT_CLASS_CATEGORY(__category) virtual auto getEventCategory() const -> int32 override {return __category;}

	class TST_LIB_API Event
	{
	public:
		virtual ~Event() = default;

		[[nodiscard]] virtual auto getEventType() const -> EEventType = 0;
		[[nodiscard]] virtual auto getEventName() const -> CString = 0;
		[[nodiscard]] virtual auto getEventCategory() const -> int32 = 0;

		[[nodiscard]] auto inCategory(const EventCategory p_category) const -> bool { return getEventCategory() & p_category; }

		[[nodiscard]] auto isHandled() const -> bool { return m_handled; }
		auto               setHandled(const bool p_handled) -> void { m_handled = p_handled; }

	protected:
		bool m_handled{false};

		friend class EventDispatcher;
	};

	// std::bind is not good, so I use this instead...
	#define TST_BIND_EVENT_FN(__func) [this](auto &p_event) mutable -> bool { return __func(p_event); }

	template<typename Type>
	using EventFunc = std::function<bool(Type &)>; // As much as I hate std::function, I think this is an acceptable use case

	// EventDispatcher is a utility class that allows for easy dispatching of events
	// It takes an event and a function, and calls the function if the event type matches
	class TST_LIB_API EventDispatcher
	{
	public:
		// Constructor that takes an event reference
		explicit EventDispatcher(Event &p_event) : m_event(p_event)
		{
		}

		// Dispatch function that takes a function and calls it if the event type matches
		template<typename Type> requires std::derived_from<Type, Event>
		auto dispatch(EventFunc<Type> p_func) -> bool
		{
			// Check if the event type matches the type T
			if (m_event.getEventType() == Type::getStaticType())
			{
				// Cast the event to type T and call the function with it
				m_event.m_handled = p_func(static_cast<Type &>(m_event));
				return true;
			}

			return false;
		}

	private:
		Event &m_event; // Only time I have ever used a reference member variable
	};

	using EventCallbackFn = std::function<void(Event &)>;
}
