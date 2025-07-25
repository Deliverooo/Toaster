#pragma once
//#include <glm/glm.hpp>
#include <random>

#include "Toaster.h"
#include "imgui.h"

struct ParticleCreateInfo
{
	glm::vec2 Position{ 0.0f, 0.0f };
	glm::vec2 Velocity{ 0.0f, 0.0f };
	glm::vec2 VelocityVariation{ 0.0f };

	glm::vec4 ColourBegin{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 ColourEnd{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 ColourVariation{ 0.0f };

	glm::vec2 SizeBegin{ 1.0f };
	glm::vec2 SizeEnd{ 1.0f };
	glm::vec2 SizeVariation{ 0.0f };

	float Lifetime{ 1.0f };
};

class ParticleSystem
{
public:
	ParticleSystem();

	void emit(const ParticleCreateInfo& createInfo);

	void onUpdate(tst::DeltaTime dt);
	void onRender();

	void reset();

	uint32_t m_ParticleIndex = 499;


private:
	struct Particle
	{
		glm::vec2 Position;
		glm::vec2 Velocity;

		glm::vec4 ColourBegin;
		glm::vec4 ColourEnd;

		float Rotation{ 0.0f };
		glm::vec2 SizeBegin;
		glm::vec2 SizeEnd;

		float Lifetime = 1.0f;
		float LifeRemaining = 0.0f;

		bool Active = false;
	};

	std::vector<Particle> m_Particles;
};