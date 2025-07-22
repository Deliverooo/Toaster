#pragma once
#include <random>
#include <Toaster.h>

class Player
{
public:

	Player() = default;
	~Player() = default;

	void init();

	void onUpdate(tst::DeltaTime dt);
	void onRender();

	void onImGuiRender();
	void onEvent(tst::Event& e);

	bool onWindowResizedEvent(tst::WindowResizedEvent& e);
	bool onKeyPressedEvent(tst::KeyPressedEvent& e);

	const tst::ScopedPtr<tst::OrthoCamera2D>& getCamera() { return m_Camera; }

private:

	glm::vec2 m_Position{0.0f, 1.0f};
	glm::vec2 m_Velocity{0.0f, 0.0f};

	float m_Gravity{ 3.301f };
	float m_Acceleration{ 4.0f };

	float m_Speed{ 2.124f };
	float m_JumpHeight{ 2.232f };

	float m_aspectRatio{ 1.77f };
	float m_cameraZoom{ 1.0f };

	tst::ScopedPtr<tst::OrthoCamera2D> m_Camera;

	tst::RefPtr<tst::Texture2D> m_Texture0;

};

