#include "Player.hpp"

void Player::init()
{
	m_Camera = std::make_unique<tst::OrthoCamera2D>(-m_aspectRatio * m_cameraZoom, m_aspectRatio * m_cameraZoom, -m_cameraZoom, m_cameraZoom);

	m_Texture0 = tst::Texture2D::create("assets/textures/One_Leg_Bird.png");
}

void Player::onUpdate(tst::DeltaTime dt)
{

	m_Position.y -= m_Gravity * dt;
	m_Position.y += m_Speed * m_Velocity.y * dt.getTime_s();
	m_Velocity.y *= 1.0f - dt.getTime_s();

	if (m_Position.y < -1.0f)
	{
		m_Position.y = -1.0f;
	}


	m_Camera->setPosition({m_Position.x, 0.0f});
}

void Player::onRender()
{

}

void Player::onEvent(tst::Event& e)
{
	tst::EventDispatcher eventDispatcher(e);

	eventDispatcher.dispatch<tst::WindowResizedEvent>([this](tst::WindowResizedEvent& e)
		{
			return onWindowResizedEvent(e);
		});
	eventDispatcher.dispatch<tst::KeyPressedEvent>([this](tst::KeyPressedEvent& e)
		{
			return onKeyPressedEvent(e);
		});
}

bool Player::onWindowResizedEvent(tst::WindowResizedEvent& e)
{

	m_aspectRatio = static_cast<float>(e.getWidth()) / static_cast<float>(e.getHeight());
	m_Camera->recalculateProjectionMatrix(-m_aspectRatio * m_cameraZoom, m_aspectRatio * m_cameraZoom, -m_cameraZoom, m_cameraZoom);

	return false;
}

bool Player::onKeyPressedEvent(tst::KeyPressedEvent &e)
{
	if (e.getKeycode() == TST_KEY_SPACE)
	{
		m_Velocity = glm::vec2(0.0f, m_JumpHeight);
	}
	return false;
}

void Player::onImGuiRender()
{

	ImGui::Begin("Player Settings");

	ImGui::SliderFloat("Player Speed", &m_Speed, 0.2f, 3.5f);
	ImGui::Spacing();

	ImGui::SliderFloat("Player Jump Height", &m_JumpHeight, 0.2f, 3.5f);
	ImGui::Spacing();

	ImGui::SliderFloat("Player Gravity", &m_Gravity, 0.2f, 3.5f);
	ImGui::Spacing();

	ImGui::End();
}
