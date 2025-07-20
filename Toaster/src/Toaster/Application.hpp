#pragma once

#include "Core.hpp"
#include "LayerStack.hpp"
#include "Window.hpp"
#include "Core/Time.hpp"
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

		void pushLayer(RefPtr<Layer> layer);
		void pushOverlay(RefPtr<Layer> overlay);
		void popLayer(RefPtr<Layer> layer);
		void popOverlay(RefPtr<Layer> overlay);

		Window& getWindow() { return *m_window; }

		static Application& getInstance() {
			TST_ASSERT(m_instance != nullptr, "APPLICATION ERROR -> Tried to access application instance before initialization!");
			return *m_instance;
		}

		static bool uiMode;
	private:
		bool m_running;
		static Application* m_instance;
		ScopedPtr<Window> m_window;

		LayerStack m_layerStack;
		RefPtr<ImguiLayer> m_imguiLayer;

		float m_lastFrameTime{0.0f};

	};

	// To be defined in CLIENT
	extern Application* CreateApplication();

}
