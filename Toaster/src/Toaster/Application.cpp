#include "tstpch.h"

#include "Application.hpp"

#include "Input.hpp"
#include "Events/ApplicationEvent.hpp"

#ifdef TST_PLATFORM_WINDOWS
#include "Window/Platform/Windows/WindowsWindow.hpp"
#endif

namespace tst
{
	Application* Application::m_instance = nullptr;

	Application::Application() {

		m_instance = this;

		m_window = std::unique_ptr<Window>(Window::Create({1280, 720, "Toester"}));
		m_window->setEventCallback([this](Event &e)
		{
				onEvent(e);
		});
	}

	Application::~Application() {
		// Destructor implementation
	}

	// Event handler for the application
	// This function is called whenever an event occurs
	// It logs the event and can be extended to handle specific events
	void Application::onEvent(Event& event)
	{

		EventDispatcher event_dispatcher(event);
		event_dispatcher.dispatch<WindowClosedEvent>([this](WindowClosedEvent &e){
			m_running = false;
			return true;
		});

		// Iterate through the layer stack in reverse order
		// This allows the topmost layer to handle the event first
		// If the event is handled, we stop further propagation
		// This is useful for layered applications where the top layer might handle input events
		// and prevent them from reaching lower layers
		for (auto it = m_layerStack.end(); it != m_layerStack.begin();)
		{
			(*--it)->onEvent(event);
			if (event.isHandled())
			{
				break;
			}
		}
	}

	void Application::run() {



		m_running = true;

		while (m_running)
		{
			glClearColor(0.0f, 0.31f, 0.63f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			for (Layer* layer : m_layerStack)
			{
				layer->onUpdate();
			}

			m_window->update();
		}

	}

	void Application::pushLayer(Layer* layer)
	{
		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}

	void Application::pushOverlay(Layer* overlay)
	{
		m_layerStack.pushOverlay(overlay);
		overlay->onAttach();
	}



}
