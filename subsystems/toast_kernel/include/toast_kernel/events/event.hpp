#pragma once

#include "../toast_kernel.hpp"

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
		eWindowFileDrop,
		eKeyPressed,
		eKeyReleased
	};

	#define EVENT_CLASS_TYPE(__type) static auto getStaticType() -> EEventType {return EEventType::__type;}\
							   auto getEventType() const -> EEventType override {return getStaticType();}\
							   auto getEventName() const -> CString override {return #__type;}

	class TST_KERNEL_API Event
	{
	public:
		virtual ~Event() = default;

		[[nodiscard]] virtual auto getEventType() const -> EEventType = 0;
		[[nodiscard]] virtual auto getEventName() const -> CString = 0;

		[[nodiscard]] auto isHandled() const -> bool { return m_handled; }
		auto               setHandled(const bool p_handled) -> void { m_handled = p_handled; }

	protected:
		bool m_handled{false};

		friend class EventDispatcher;
	};

	// std::bind is not good, so I use this instead...
	#define TST_BIND_EVENT_FN(__func) [this](auto &p_event) mutable -> bool { return __func(p_event); }

	// EventDispatcher is a utility class that allows for easy dispatching of events
	// It takes an event and a function, and calls the function if the event type matches
	class TST_KERNEL_API EventDispatcher
	{
	public:
		// Constructor that takes an event reference
		explicit EventDispatcher(Event &p_event) : m_event(p_event)
		{
		}

		// Dispatch function that takes a function and calls it if the event type matches
		template<typename Type, typename TFunc> requires std::derived_from<Type, Event>
		auto dispatch(TFunc &&p_func) -> bool
		{
			// Check if the event type matches the type
			if (m_event.getEventType() == Type::getStaticType())
			{
				// Cast the event to type and call the function with it
				m_event.m_handled = p_func(static_cast<Type &>(m_event));
				return true;
			}

			return false;
		}

	private:
		Event &m_event; // Only time I have ever used a reference member variable
	};

	using EventCallbackFn = void(*)(Event &, void *);
}
