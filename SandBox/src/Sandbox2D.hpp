#pragma once

#include <random>

#include "Toaster.h"
#include "imgui.h"
#include "Toaster/CameraController.hpp"

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

struct Particle2D
{
	glm::vec2 position{ 0.0f };
	glm::vec2 velocity{ 0.0f };
};

class SandBox2DLayer : public tst::Layer
{
public:
	SandBox2DLayer() = default;

	virtual ~SandBox2DLayer() override = default;

	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(tst::DeltaTime dt) override;
	virtual void onEvent(tst::Event& e) override;
	virtual void onImguiRender() override;

private:


	tst::RefPtr<tst::OrthoCamera2DController> m_OrthoCameraCtrl;

	tst::ShaderLib m_ShaderLibrary;

	tst::RefPtr<tst::VertexArray> m_Vao;

	tst::RefPtr<tst::Texture2D> m_Texture0;
	tst::RefPtr<tst::Texture2D> m_Texture1;


	glm::vec4 m_quadColour{ 1.0f, 1.0f, 1.0f, 1.0f };

	glm::vec4 m_clearColour{ 0.15f, 0.15f, 0.15f, 0.0f };

	Transform2D m_modelTransform = {
		glm::vec2{0.0f, 0.0f}, // position
		glm::vec2{1.0f, 1.0f},
		180.0f
	};

};
