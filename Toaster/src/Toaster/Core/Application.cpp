#include "tstpch.h"

#include "Application.hpp"
#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Renderer/Renderer.hpp"

#ifdef TST_PLATFORM_WINDOWS
#include "platform/Windows/WindowsWindow.hpp"
#endif

namespace tst
{
	Application* Application::m_instance = nullptr;
	bool Application::uiMode = false;
	
	void Application::init()
	{
		TST_PROFILE_FN();

		m_window = ScopedPtr<Window>(Window::Create({ 1280, 720, "Toaster", true }));
		m_window->setEventCallback([this](Event& e)
			{
				onEvent(e);
			});

		Renderer::init();

		m_imguiLayer = std::make_shared<ImguiLayer>();
		pushOverlay(m_imguiLayer);
	}

	Application::Application() {

		m_instance = this;

		init();
	}

	Application::~Application() {

		Renderer::terminate();
		// Destructor implementation
	}

	// Event handler for the application
	// This function is called whenever an event occurs
	// It logs the event and can be extended to handle specific events
	void Application::onEvent(Event& event)
	{
		TST_PROFILE_FN();

		EventDispatcher event_dispatcher(event);
		event_dispatcher.dispatch<WindowClosedEvent>([this](WindowClosedEvent &e){
			m_running = false;
			return true;
		});
		event_dispatcher.dispatch<WindowResizedEvent>([this](WindowResizedEvent& e) {
			{
				TST_PROFILE_SCP("On window resize - App");

				Renderer::resizeViewport(e.getWidth(), e.getHeight());
			}
			return false;
		});
		event_dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
			if (e.getKeycode() == TST_KEY_ESCAPE)
			{
				m_running = false;
			}
			if (e.getKeycode() == TST_KEY_I)
			{
				if (uiMode)
				{
					glfwSetInputMode((GLFWwindow*)m_window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					uiMode = false;
					return false;
				} else
				{
					glfwSetInputMode((GLFWwindow*)m_window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					uiMode = true;
					return false;
				}
			}
			if (e.getKeycode() == TST_KEY_F11)
			{
				/*static bool fullscreen = false;

				if (!fullscreen)
				{
					const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
					glfwSetWindowMonitor((GLFWwindow*)m_window->getWindow(), glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);

					fullscreen = true;
				} else
				{
					int xpos, ypos;
					glfwGetWindowPos((GLFWwindow*)m_window->getWindow(), &xpos, &ypos);
					int xscale, yscale;
					glfwGetWindowSize((GLFWwindow*)m_window->getWindow(), &xscale, &yscale);
					glfwSetWindowMonitor((GLFWwindow*)m_window->getWindow(), nullptr, xpos, ypos, xscale, yscale, 0);

					fullscreen = false;
				}*/

			}
			return false;
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

		TST_PROFILE_FN();

		while (m_running)
		{

			float currentTime = static_cast<float>(glfwGetTime());
			DeltaTime dt = { m_lastFrameTime, currentTime };
			m_lastFrameTime = currentTime;

			

			for (RefPtr<Layer> layer : m_layerStack)
			{
				layer->onUpdate(dt);
			}

			m_imguiLayer->begin();
			for (RefPtr<Layer> layer : m_layerStack)
			{
				layer->onImguiRender();
			}
			m_imguiLayer->end();

			m_window->update();
		}

	}

	void Application::pushLayer(RefPtr<Layer> layer)
	{
		TST_PROFILE_FN();

		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}
	void Application::pushOverlay(RefPtr<Layer> overlay)
	{
		TST_PROFILE_FN();

		m_layerStack.pushOverlay(overlay);
		overlay->onAttach();
	}
	void Application::popLayer(RefPtr<Layer> layer)
	{
		m_layerStack.popLayer(layer);
		layer->onDetach();
	}
	void Application::popOverlay(RefPtr<Layer> overlay)
	{
		m_layerStack.popOverlay(overlay);
		overlay->onDetach();
	}


}
