#pragma once

#include "Core.hpp"
#include "LayerStack.hpp"
#include "Window.hpp"
#include "Events/Event.hpp"
#include "imgui/ImguiLayer.hpp"
#include "platform/OpenGl/OpenGlTestLayer.h"
#include "Renderer/Shader.hpp"


namespace tst {

	class TST_API Application
	{
	public:
		Application();
		~Application();

		void init();
		void run();

		void onEvent(Event &event);

		void pushLayer(Layer* layer);
		void pushOverlay(Layer* overlay);
		void popLayer(Layer* layer);
		void popOverlay(Layer* overlay);

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
		ImguiLayer *m_imguiLayer = nullptr;
		OpenGlTestLayer *m_openGlTestLayer = nullptr;

		unsigned int m_Vao;
		unsigned int m_Vbo;
		unsigned int m_Ebo;

		std::unique_ptr<Shader> m_basicShader;
	};

	// To be defined in CLIENT
	extern Application* CreateApplication();

}
