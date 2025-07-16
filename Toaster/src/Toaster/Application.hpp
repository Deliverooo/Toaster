#pragma once

#include "Core.hpp"
#include "LayerStack.hpp"
#include "Window.hpp"
#include "Events/Event.hpp"
#include "imgui/ImguiLayer.hpp"
#include "Renderer/Shader.hpp"
#include "Renderer/Buffer.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Texture.hpp"
#include "Renderer/VertexArray.hpp"


namespace tst {

	class TST_API Application
	{
	public:
		Application();
		~Application();

		void init();
		void run();

		void onEvent(Event &event);

		void pushLayer(std::shared_ptr<Layer> layer);
		void pushOverlay(std::shared_ptr<Layer> overlay);
		void popLayer(std::shared_ptr<Layer> layer);
		void popOverlay(std::shared_ptr<Layer> overlay);

		Window& getWindow() { return *m_window; }

		static Application& getInstance() {
			TST_ASSERT(m_instance != nullptr, "APPLICATION ERROR -> Tried to access application instance before initialization!");
			return *m_instance;
		}

	private:
		bool m_running;
		static Application* m_instance;
		std::unique_ptr<Window> m_window;

		LayerStack m_layerStack;
		std::shared_ptr<ImguiLayer> m_imguiLayer;

		
	};

	// To be defined in CLIENT
	extern Application* CreateApplication();

}
