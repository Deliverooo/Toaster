#pragma once
#include <random>

#include "Toaster.h"
#include "imgui.h"
#include "Toaster/CameraController.hpp"


namespace tst
{

	class RayTracerLayer : public Layer
	{
	public:
		RayTracerLayer();

		virtual ~RayTracerLayer() override = default;

		virtual void onAttach() override;
		virtual void onDetach() override {}
		virtual void onUpdate(DeltaTime dt) override;
		virtual void onEvent(Event& e) override;
		virtual void onImguiRender() override;

	private:
		glm::vec4 perPixel(glm::vec2 coord);

		void loadAssets();
		bool onKeyPressedEvent(KeyPressedEvent& e);

		void render();

		PerspectiveCameraController m_PerspectiveCameraCtrl;

		RefPtr<SubTexture2D> m_Texture0;
		RefPtr<Texture2D> m_renderTexture;
		uint32_t* m_imageData = nullptr;
		RefPtr<Framebuffer> m_Framebuffer;


		glm::vec4 m_clearColour{ 0.15f, 0.15f, 0.15f, 1.0f };
		glm::vec2 m_ViewportSize{ 1280, 720 };

		uint32_t m_ViewportWidth{ 1280 };
		uint32_t m_ViewportHeight{ 720 };

		float m_renderTime{ 0.0f };
	};
}