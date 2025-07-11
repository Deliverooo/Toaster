#pragma once

#include "Core.hpp"
#include "LayerStack.hpp"
#include "Window.hpp"
#include "Events/Event.hpp"


namespace tst {

	class TST_API Application
	{
	public:
		Application();
		~Application();

		void run();

		void onEvent(Event &event);

		void pushLayer(Layer* layer);
		void pushOverlay(Layer* overlay);

		Window& getWindow() { return *m_window; }

		static Application& getInstance() {
			TST_ASSERT(m_instance != nullptr, "APPLICATION ERROR -> Tried to access application instance before initialization!");
			return *m_instance;
		}

	private:
		std::unique_ptr<Window> m_window;
		bool m_running;
		LayerStack m_layerStack;

		static Application* m_instance;
	};

	// To be defined in CLIENT
	extern Application* CreateApplication();

}
