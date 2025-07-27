#pragma once
#include "Toaster/Core/Window.hpp"

#define TST_USE_VULKAN

#ifdef TST_USE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include "GLFW/glfw3.h"

#include "platform/OpenGl/OpenGLRenderingContext.hpp"


namespace tst
{
	// Windows-specific implementation of the Window interface
	class WindowsWindow : public Window
	{
	public:
		// Constructs a window with the given attributes (width, height, title)
		WindowsWindow(const WindowAttribArray& window_attributes);

		~WindowsWindow() override;

		virtual void shutdown() override;

		// Updates the window (polls events, swaps buffers, etc.)
		virtual void update() override;

		virtual unsigned int getWidth() const override;
		virtual unsigned int getHeight() const override;
		virtual const char* getTitle() const override;
		virtual float getAspect() const override;

		virtual bool getVsyncEnabled() const override;
		virtual void enableVsync(bool yn) override;

		virtual std::pair<int, int> getPosition() const override;

		// Sets the callback function for handling events
		// The callback should accept an Event reference and return void
		virtual void setEventCallback(std::function<void(Event&)> callback) override { m_windowData.eventCallback = callback; }

		// Uncomment to enable VSync functionality
		// void enableVsync(const bool enabled) override;
		// bool getVsyncEnabled() const override;

		virtual void* getWindow() override { return m_window; };

	private:

		GLFWimage loadIcon(const char* filepath);

		RenderingContext* m_renderingContext;

		// Holds window properties and event callback
		struct WindowData
		{
			unsigned int width;
			unsigned int height;
			const char* title;
			std::function<void(Event&)> eventCallback;
			bool vSyncEnabled; // Uncomment if VSync is needed
		};

		WindowData m_windowData;      // Current window data
		GLFWwindow* m_window;       // Pointer to the GLFW window

		std::pair<int, int> m_windowPos;
	};
}
