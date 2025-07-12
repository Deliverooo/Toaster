#pragma once
#include "Toaster/Window.hpp"
#include "Glad/glad.h"
#include "GLFW/glfw3.h"

#ifdef TST_PLATFORM_WINDOWS

namespace tst
{
	// Windows-specific implementation of the Window interface
	class WindowsWindow : public Window
	{
	public:
		// Constructs a window with the given attributes (width, height, title)
		WindowsWindow(const WindowAttribArray& window_attributes);

		// Cleans up window resources
		~WindowsWindow();

		// Updates the window (polls events, swaps buffers, etc.)
		void update() override;

		// Returns the window width in pixels
		unsigned int getWidth() const override;

		// Returns the window height in pixels
		unsigned int getHeight() const override;

		// Returns the window title
		const char* getTitle() const override;

		// Sets the callback function for handling events
		// The callback should accept an Event reference and return void
		void setEventCallback(std::function<void(Event&)> callback) override { m_windowData.eventCallback = callback; }

		// Uncomment to enable VSync functionality
		// void enableVsync(const bool enabled) override;
		// bool getVsyncEnabled() const override;

		void* getWindow() override { return m_window; };

	private:
		// Holds window properties and event callback
		struct WindowData
		{
			unsigned int width;
			unsigned int height;
			const char* title;
			std::function<void(Event&)> eventCallback;
			// bool vSyncEnabled{ true }; // Uncomment if VSync is needed
		};

		WindowData m_windowData;      // Current window data
		GLFWwindow* m_window;         // Pointer to the GLFW window
	};
}

#endif