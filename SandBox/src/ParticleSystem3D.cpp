#include "ParticleSystem3D.hpp"

#include "Utils/Random.hpp"

ParticleSystem3D::ParticleSystem3D()
{
	m_Particles.resize(1000);
}

void ParticleSystem3D::emit(const Particle3DCreateInfo& createInfo)
{
	Particle3D& particle = m_Particles[m_ParticleIndex];
	particle.Active = true;
	particle.Position = createInfo.Position;
	particle.Rotation = glm::vec3(Random::Float() * 2.0f * glm::pi<float>());

	// Velocity
	particle.Velocity = createInfo.Velocity;
	particle.Velocity.x += createInfo.VelocityVariation.x * (Random::Float() - 0.5f);
	particle.Velocity.y += createInfo.VelocityVariation.y * (Random::Float() - 0.5f);
	particle.Velocity.z += createInfo.VelocityVariation.z * (Random::Float() - 0.5f);

	// Colour
	//particle.ColourBegin = createInfo.ColourBegin;
	particle.ColourBegin.r = createInfo.ColourBegin.r + createInfo.ColourVariation.r * (Random::Float() - 0.5f);
	particle.ColourBegin.g = createInfo.ColourBegin.g + createInfo.ColourVariation.g * (Random::Float() - 0.5f);
	particle.ColourBegin.b = createInfo.ColourBegin.b + createInfo.ColourVariation.b * (Random::Float() - 0.5f);
	particle.ColourBegin.a = createInfo.ColourBegin.a + createInfo.ColourVariation.a * (Random::Float() - 0.5f);

	particle.ColourEnd = createInfo.ColourEnd;

	// Size
	particle.SizeBegin = glm::vec3(createInfo.SizeBegin + createInfo.SizeVariation * (Random::Float() - 0.5f));
	particle.SizeEnd = createInfo.SizeEnd;

	// Life
	particle.Lifetime = createInfo.Lifetime;
	particle.LifeRemaining = createInfo.Lifetime;

	m_ParticleIndex = --m_ParticleIndex % m_Particles.size();

	TST_INFO("Particle emitted at: [{0}, {1}, {2}]", particle.Position.x, particle.Position.y, particle.Position.z);

}

void ParticleSystem3D::onUpdate(tst::DeltaTime dt)
{
	for (auto& particle : m_Particles)
	{
		if (!particle.Active)
			continue;

		if (particle.LifeRemaining <= 0.0f)
		{
			particle.Active = false;
			continue;
		}

		particle.LifeRemaining -= dt;
		particle.Position += particle.Velocity * static_cast<float>(dt);
		particle.Rotation += glm::vec3(0.01f * dt);
	}
}

void ParticleSystem3D::onRender()
{
	for (auto& particle : m_Particles)
	{
		if (!particle.Active)
			continue;

		float lifePercent = particle.LifeRemaining / particle.Lifetime;
		glm::vec4 Colour = glm::mix(particle.ColourEnd, particle.ColourBegin, lifePercent);

		// Fades the particle based on its life remaining
		Colour.a *= lifePercent;

		glm::vec3 size = glm::mix(particle.SizeEnd, particle.SizeBegin, lifePercent);
		tst::Renderer3D::drawCube(particle.Position, particle.Rotation, size, Colour);
	}
}