#include "Sandbox2D.hpp"


SandBox2DLayer::SandBox2DLayer() : m_OrthoCameraCtrl(1.77f)
{
	
}

void SandBox2DLayer::onAttach()
{

	m_Texture0 = tst::Texture2D::create("assets/textures/orbo0.png");

}
void SandBox2DLayer::onDetach()
{

}

void SandBox2DLayer::onUpdate(tst::DeltaTime dt)
{
	//m_ParticleSystem.onUpdate(dt);

	tst::RenderCommand::setClearColour(m_clearColour); 
	tst::RenderCommand::clear();


	tst::Renderer2D::begin(m_OrthoCameraCtrl.getCamera());

	m_OrthoCameraCtrl.onUpdate(dt);

	tst::Renderer2D::drawQuad({ 1.0f, 1.0f, 0.0f }, 1.0f, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });

	tst::Renderer2D::drawQuad({ 0.0f, 0.0f, 0.0f }, 1.0f, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });



	tst::Renderer2D::end();

}
void SandBox2DLayer::onEvent(tst::Event& e)
{
	m_OrthoCameraCtrl.onEvent(e);

	tst::EventDispatcher eventDispatcher(e);

	eventDispatcher.dispatch<tst::KeyPressedEvent>([this](tst::KeyPressedEvent& e)
		{
			return onKeyPressedEvent(e);
		});
	eventDispatcher.dispatch<tst::KeyHeldEvent>([this](tst::KeyHeldEvent& e)
		{
			return onKeyHeldEvent(e);
		});

}

bool SandBox2DLayer::onKeyPressedEvent(tst::KeyPressedEvent& e)
{

	if (e.getKeycode() == TST_KEY_E)
	{
		ParticleCreateInfo particle_create_info{};
		particle_create_info.Position = m_OrthoCameraCtrl.getPosition() + glm::vec2(sin(m_OrthoCameraCtrl.getRotation()) * 0.5f);
		particle_create_info.Velocity = m_OrthoCameraCtrl.getVelocity();

		particle_create_info.SizeBegin = { 0.2f, 0.2f };
		particle_create_info.SizeEnd = { 0.025f, 0.025f };
		particle_create_info.SizeVariation = { 0.1f, 0.1f };

		particle_create_info.ColourBegin = { 1.0f, 0.2f, 0.2f, 1.0f };
		particle_create_info.ColourEnd = { 0.2f, 0.0f, 0.0f, 1.0f };
		particle_create_info.ColourVariation = { 0.35f, 0.325f, 0.025f, 0.0f };

		particle_create_info.Lifetime = 2.0f;

		m_ParticleSystem.emit(particle_create_info);
	}

	return false;
}

bool SandBox2DLayer::onKeyHeldEvent(tst::KeyHeldEvent& e)
{


	return false;
}


void SandBox2DLayer::onImguiRender()
{

}