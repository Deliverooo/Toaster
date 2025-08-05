#include "tstpch.h"

#include "Application.hpp"
#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Renderer/MeshRenderer.hpp"
#include "Toaster/Renderer/Renderer.hpp"
#include "Toaster/Renderer/Renderer2D.hpp"
#include "Toaster/Renderer/Renderer3D.hpp"
#include "Toaster/Renderer/SkyBoxRenderer.hpp"

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

		m_window = ScopedPtr<Window>(Window::Create({ 1920, 1080, "Toaster", true }));
		m_window->setEventCallback([this](Event& e)
			{
				onEvent(e);
			});

		MeshRenderer::init();
		Renderer2D::init();
		SkyBoxRenderer::init();

		m_imguiLayer = std::make_shared<ImguiLayer>();
		pushOverlay(m_imguiLayer);
	}

	Application::Application() {

		m_instance = this;

		init();
	}

	Application::~Application() {

		Renderer2D::terminate();
		MeshRenderer::terminate();
		SkyBoxRenderer::terminate();
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
				if (e.getWidth() == 0 || e.getHeight() == 0)
				{
					m_minimized = true;
					return false;
				}

				m_minimized = false;

				Renderer::resizeViewport(e.getWidth(), e.getHeight());
			}
			return false;
		});
		event_dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {


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

			if (!m_minimized)
			{
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
			}

			m_window->update();
		}


	}

	void Application::close()
	{
		m_running = false;
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
