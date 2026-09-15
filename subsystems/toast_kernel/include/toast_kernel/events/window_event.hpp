#pragma once

#include <vector>

#include "event.hpp"

namespace toaster
{
	class TST_LIB_API WindowCloseEvent final : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_CATEGORY(EventCategory_Application)
		EVENT_CLASS_TYPE(eWindowClose)
	};

	class TST_LIB_API WindowResizeEvent final : public Event
	{
	public:
		WindowResizeEvent(tsm::uint2 p_size) : m_size(p_size)
		{
		}

		EVENT_CLASS_CATEGORY(EventCategory_Application)
		EVENT_CLASS_TYPE(eWindowResize)

		[[nodiscard]] auto getSize() const -> tsm::uint2 { return m_size; }
		[[nodiscard]] auto getAspectRatio() const -> float32 { return m_size.aspect(); }

	private:
		tsm::uint2 m_size;
	};

	class TST_LIB_API WindowMinimizeEvent final : public Event
		{
	public:
		explicit WindowMinimizeEvent(const bool minimized) : m_minimized(minimized)
		{
		}

		EVENT_CLASS_CATEGORY(EventCategory_Application)
		EVENT_CLASS_TYPE(eWindowMinimize)

		[[nodiscard]] auto isMinimized() const -> bool { return m_minimized; }

	private:
		bool m_minimized;
	};

	class TST_LIB_API WindowMaximizeEvent final : public Event
	{
	public:
		explicit WindowMaximizeEvent(const bool p_maximized) : m_maximized(p_maximized)
		{
		}

		EVENT_CLASS_CATEGORY(EventCategory_Application)
		EVENT_CLASS_TYPE(eWindowMaximize)

		[[nodiscard]] auto isMaximized() const -> bool { return m_maximized; }

	private:
		bool m_maximized{true};
	};

	class TST_LIB_API WindowFileDropEvent final : public Event
	{
	public:
		WindowFileDropEvent(const std::vector<String> &p_filepaths) : m_filepaths(p_filepaths)
		{
		}

		EVENT_CLASS_CATEGORY(EventCategory_Application)
		EVENT_CLASS_TYPE(eWindowFileDrop)

		[[nodiscard]] auto getFilepaths() const -> const std::vector<String> & { return m_filepaths; }

	private:
		std::vector<String> m_filepaths;
	};
}
