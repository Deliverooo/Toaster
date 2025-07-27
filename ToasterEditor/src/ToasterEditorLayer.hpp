#pragma once
#include <random>

#include "Toaster.h"
#include "imgui.h"
#include "Toaster/CameraController.hpp"


namespace tst
{

	struct Particle3DCreateInfo
	{
		glm::vec3 Position{ 0.0f };
		glm::vec3 Velocity{ 0.0f };
		glm::vec3 VelocityVariation{ 0.0f };

		glm::vec4 ColourBegin{ 1.0f };
		glm::vec4 ColourEnd{ 1.0f };
		glm::vec4 ColourVariation{ 0.0f };

		glm::vec3 SizeBegin{ 1.0f };
		glm::vec3 SizeEnd{ 1.0f };
		glm::vec3 SizeVariation{ 0.0f };

		float Lifetime{ 1.0f };
	};

	class ParticleSystem3D
	{
	public:
		ParticleSystem3D();

		void emit(const Particle3DCreateInfo& createInfo);

		void onUpdate(tst::DeltaTime dt);
		void onRender();

	private:
		struct Particle3D
		{
			glm::vec3 Position;
			glm::vec3 Velocity;

			glm::vec4 ColourBegin;
			glm::vec4 ColourEnd;

			glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
			glm::vec3 SizeBegin;
			glm::vec3 SizeEnd;

			float Lifetime{ 1.0f };
			float LifeRemaining{ 0.0f };

			bool Active{ false };
		};

		std::vector<Particle3D> m_Particles;
		uint32_t m_ParticleIndex = 499;
	};

	class ToasterEditorLayer : public Layer
	{
	public:
		ToasterEditorLayer();

		virtual ~ToasterEditorLayer() override = default;

		virtual void onAttach() override;
		virtual void onDetach() override {}
		virtual void onUpdate(DeltaTime dt) override;
		virtual void onEvent(Event& e) override;
		virtual void onImguiRender() override;

	private:

		bool onKeyPressedEvent(KeyPressedEvent& e);

		glm::vec4 screenSpaceToWorldSpace(glm::vec2 screenCoords, float depth);

		OrthoCamera2DController m_OrthoCameraCtrl;
		PerspectiveCameraController m_PerspectiveCameraCtrl;

		RefPtr<Texture2D> m_OrboTexture;
		RefPtr<Texture2D> m_GrassTexture;
		RefPtr<Texture2D> m_BirdTexture;
		RefPtr<Texture2D> m_RayTraceRoomTexture;

		RefPtr<SubTexture2D> m_Texture0;

		RefPtr<Framebuffer> m_Framebuffer;

		glm::vec4 m_clearColour{ 0.15f, 0.15f, 0.15f, 1.0f };

		glm::vec2 m_ViewportSize{1280, 720};

		ParticleSystem3D m_particleSystem;


		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;

		bool m_particleDrawMode = false;

	};
}