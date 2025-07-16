#include "tstpch.h"

#include "Application.hpp"
#include "Events/ApplicationEvent.hpp"

#ifdef TST_PLATFORM_WINDOWS
#include "platform/Windows/WindowsWindow.hpp"
#endif

namespace tst
{
	Application* Application::m_instance = nullptr;

	
	void Application::init()
	{
		m_window = std::unique_ptr<Window>(Window::Create({ 1280, 720, "Toaster", true}));
		m_window->setEventCallback([this](Event& e)
			{
				onEvent(e);
			});

		m_imguiLayer = std::make_shared<ImguiLayer>();
		pushOverlay(m_imguiLayer);

		glEnable(GL_DEPTH_TEST);
	}

	Application::Application() {

		m_instance = this;

		init();
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
		event_dispatcher.dispatch<WindowResizedEvent>([this](WindowResizedEvent& e) {
			glViewport(0, 0, static_cast<int>(m_window->getWidth()), static_cast<int>(m_window->getHeight()));
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

			for (std::shared_ptr<Layer> layer : m_layerStack)
			{
				layer->onUpdate();
			}

			m_imguiLayer->begin();
			for (std::shared_ptr<Layer> layer : m_layerStack)
			{
				layer->onImguiRender();
			}
			m_imguiLayer->end();

			m_window->update();
		}

	}

	void Application::pushLayer(std::shared_ptr<Layer> layer)
	{
		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}
	void Application::pushOverlay(std::shared_ptr<Layer> overlay)
	{
		m_layerStack.pushOverlay(overlay);
		overlay->onAttach();
	}
	void Application::popLayer(std::shared_ptr<Layer> layer)
	{
		m_layerStack.popLayer(layer);
		layer->onDetach();
	}
	void Application::popOverlay(std::shared_ptr<Layer> overlay)
	{
		m_layerStack.popOverlay(overlay);
		overlay->onDetach();
	}


}
