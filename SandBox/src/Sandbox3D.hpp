#pragma once

#include <random>

#include "Toaster.h"
#include "imgui.h"
#include "ParticleSystem3D.hpp"
#include "Toaster/CameraController.hpp"

struct Transform3D
{
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f, 1.0f, 0.0f };
	glm::vec3 scale{ 1.0f };

	const glm::mat4 matrix() const
	{
		glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
		glm::mat4 rot = (glm::length(rotation) <= 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation));
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
		return trans * rot * sca;
	}

	static const glm::mat4 constructMatrix(const glm::vec3& p, const glm::vec3& r, const glm::vec3& s)
	{
		glm::mat4 sca = glm::scale(glm::mat4(1.0f), s);
		glm::mat4 rot = (glm::length(r) <= 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(r), glm::normalize(r));
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), p);
		return trans * rot * sca;
	}
};

struct Particle3D
{
	glm::vec3 position{ 0.0f };
	glm::vec3 velocity{ 0.0f };
};

class SandBox3DLayer : public tst::Layer
{
public:
	SandBox3DLayer() = default;

	virtual ~SandBox3DLayer() override = default;

	virtual void onAttach() override;
	virtual void onDetach() override;
	virtual void onUpdate(tst::DeltaTime dt) override;
	virtual void onEvent(tst::Event& e) override;
	virtual void onImguiRender() override;

private:

	bool onKeyPressedEvent(tst::KeyPressedEvent& e);
	bool onKeyHeldEvent(tst::KeyHeldEvent& e);


	glm::vec4 m_clearColour{ 0.15f };
	tst::RefPtr<tst::PerspectiveCameraController> m_PerspectiveCameraCtrl;

	tst::RefPtr<tst::Texture2D> m_Texture0;
	glm::vec4 m_CubeColour{ 1.0f, 1.0f, 1.0f, 1.0f };

};