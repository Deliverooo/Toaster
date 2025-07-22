#pragma once
//#include <glm/glm.hpp>
#include <random>

#include "Toaster.h"
#include "imgui.h"


struct Particle3DCreateInfo
{
	glm::vec3 Position{ 0.0f };
	glm::vec3 Velocity{ 0.0f };
	glm::vec3 VelocityVariation{ 0.0f };

	glm::vec4 ColourBegin{ 1.0f };
	glm::vec4 ColourEnd	 { 1.0f };
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

		glm::vec3 Rotation{ 0.0f };
		glm::vec3 SizeBegin;
		glm::vec3 SizeEnd;

		float Lifetime{ 1.0f };
		float LifeRemaining{ 0.0f };

		bool Active{ false };
	};

	std::vector<Particle3D> m_Particles;
	uint32_t m_ParticleIndex = 999;
};