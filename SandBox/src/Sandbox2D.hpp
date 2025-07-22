#pragma once
#include <random>

#include "Toaster.h"
#include "imgui.h"
#include "Toaster/CameraController.hpp"
#include "ParticleSystem.hpp"

struct Transform2D
{
	glm::vec2 position;
	glm::vec2 scale;
	float rotation;


	const glm::mat4 matrix() const
	{
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f));
		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 sca = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, 1.0f));

		return trans * rot * sca;
	}
};

class SandBox2DLayer : public tst::Layer
{
public:
	SandBox2DLayer();

	virtual ~SandBox2DLayer() override = default;

	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(tst::DeltaTime dt) override;
	virtual void onEvent(tst::Event& e) override;
	virtual void onImguiRender() override;

private:

	bool onKeyPressedEvent(tst::KeyPressedEvent& e);
	bool onKeyHeldEvent(tst::KeyHeldEvent& e);

	float m_WindowWidth{ 1920 };
	float m_WindowHeight{ 1080 };

	enum class GameState
	{
		Play = 0, MainMenu = 1, GameOver = 2
	};

	tst::OrthoCamera2DController m_OrthoCameraCtrl;

	tst::RefPtr<tst::Texture2D> m_Texture0;
	glm::vec4 m_clearColour{ 0.15f, 0.15f, 0.15f, 0.0f };

	ParticleSystem m_ParticleSystem;
};
