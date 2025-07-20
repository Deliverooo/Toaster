#include "tstpch.h"
#include "CameraController.hpp"

#include "Application.hpp"
#include "Input.hpp"
#include "glm/gtc/constants.hpp"

namespace tst
{
	//----------------------------------------------------------------
	//-----------------------Perspective Camera-----------------------
	//----------------------------------------------------------------

	PerspectiveCameraController::PerspectiveCameraController(float fov, float aspectRatio, float cameraSpeed, float acceleration, float damping)
		: m_fov(fov), m_aspectRatio(aspectRatio), m_cameraSpeed(cameraSpeed), m_acceleration(acceleration), m_damping(damping)
	{
		m_Camera = std::make_shared<PerspectiveCamera>(m_fov, m_aspectRatio);
		m_Camera->setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
		m_Camera->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
	}

	void PerspectiveCameraController::onUpdate(DeltaTime dt)
	{
		recalculateCameraVectors();

		float cameraSpeed = m_cameraSpeed * (Input::isKeyPressed(TST_KEY_LEFT_CONTROL) ? 3.0f : 1.0f);

		glm::vec3 cameraDirection{ 0.0f };
		if (Input::isKeyPressed(TST_KEY_W))			 { cameraDirection += glm::vec3(m_CameraFront.x, 0.0f, m_CameraFront.z); }
		if (Input::isKeyPressed(TST_KEY_S))			 { cameraDirection -= glm::vec3(m_CameraFront.x, 0.0f, m_CameraFront.z); }
		if (Input::isKeyPressed(TST_KEY_A))			 { cameraDirection -= glm::vec3(m_CameraRight.x, 0.0f, m_CameraRight.z); }
		if (Input::isKeyPressed(TST_KEY_D))			 { cameraDirection += glm::vec3(m_CameraRight.x, 0.0f, m_CameraRight.z); }
		if (Input::isKeyPressed(TST_KEY_SPACE))		 { cameraDirection += glm::vec3(0.0f, 1.0f, 0.0f); }
		if (Input::isKeyPressed(TST_KEY_LEFT_SHIFT)) { cameraDirection -= glm::vec3(0.0f, 1.0f, 0.0f); }

		if (glm::length(cameraDirection) > std::numeric_limits<float>::epsilon()) { cameraDirection = glm::normalize(cameraDirection); }
		
		glm::vec3 startingVelocity = cameraDirection * cameraSpeed;
		m_cameraVelocity = glm::mix(m_cameraVelocity, startingVelocity, dt * m_acceleration);
		m_cameraVelocity *= std::exp(-m_damping * dt);
		m_cameraPosition += m_cameraVelocity * dt.getTime_s();

		m_Camera->setPosition(m_cameraPosition);
		m_Camera->setRotation(m_cameraRotation);
	}
	void PerspectiveCameraController::onEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent &e)
		{
			return onKeyPressedEvent(e);
		});
		dispatcher.dispatch<KeyReleasedEvent>([this](KeyReleasedEvent&e)
		{
			return onKeyReleasedEvent(e);
		});
		dispatcher.dispatch<MouseMoveEvent>([this](MouseMoveEvent&e)
		{
			return onMouseMoveEvent(e);
		});
		dispatcher.dispatch<MouseScrollEvent>([this](MouseScrollEvent &e)
		{
			return onMouseScrollEvent(e);
		});
		dispatcher.dispatch<WindowResizedEvent>([this](WindowResizedEvent&e)
		{
			return onWindowResizedEvent(e);
		});
	}
	bool PerspectiveCameraController::onKeyPressedEvent(KeyPressedEvent& e)
	{

		return false;
	}
	bool PerspectiveCameraController::onKeyReleasedEvent(KeyReleasedEvent &e)
	{

		return false;
	}
	bool PerspectiveCameraController::onMouseMoveEvent(MouseMoveEvent& e)
	{
		static float lastX	 = 0.0f;
		static float lastY	 = 0.0f;
		static float mouseDx = 0.0f;
		static float mouseDy = 0.0f;

		mouseDx = static_cast<float>(e.getMouseX()) - lastX;
		mouseDy = static_cast<float>(e.getMouseY()) - lastY;
		lastX	= static_cast<float>(e.getMouseX());
		lastY	= static_cast<float>(e.getMouseY());

		mouseDx *= 0.1f;
		mouseDy *= 0.1f;

		if (!Application::uiMode)
		{
			m_cameraRotation.x -= glm::radians(mouseDy);
			m_cameraRotation.y += glm::radians(mouseDx);

			if (m_cameraRotation.x > glm::radians(89.0f))  { m_cameraRotation.x = glm::radians(89.0f); }
			if (m_cameraRotation.x < glm::radians(-89.0f)) { m_cameraRotation.x = glm::radians(-89.0f); }

			m_cameraRotation.y = glm::mod(m_cameraRotation.y, glm::two_pi<float>());
		}

		mouseDy = 0.0f;
		mouseDx = 0.0f;

		return false;
	}

	bool PerspectiveCameraController::onMouseScrollEvent(MouseScrollEvent& e)
	{
		m_fov -= glm::radians(static_cast<float>(e.getScrollY())) * 3.0f;

		if (m_fov < 1.0f) {
			m_fov = 1.0f;
		}
		if (m_fov > glm::radians(120.0f)) {
			m_fov = glm::radians(120.0f);
		}

		m_Camera->recalculateProjectionMatrix(m_fov, m_aspectRatio, 0.1f, 1000.0f);

		return false;
	}

	bool PerspectiveCameraController::onWindowResizedEvent(WindowResizedEvent& e)
	{
		m_aspectRatio = static_cast<float>(e.getWidth()) / static_cast<float>(e.getHeight());
		m_Camera->recalculateProjectionMatrix(m_fov, m_aspectRatio, 0.1f, 1000.0f);

		return false;
	}

	void PerspectiveCameraController::recalculateCameraVectors()
	{
		float cosPitch = glm::cos(m_cameraRotation.x);
		float sinPitch = glm::sin(m_cameraRotation.x);
		float cosYaw   = glm::cos(m_cameraRotation.y);
		float sinYaw   = glm::sin(m_cameraRotation.y);

		m_CameraFront = glm::normalize(glm::vec3(
			cosYaw * cosPitch,
			sinPitch,
			sinYaw * cosPitch
		));

		m_CameraRight = glm::normalize(glm::cross(m_CameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));

		m_CameraUp = glm::normalize(glm::cross(m_CameraRight, m_CameraFront));
	}

	//-----------------------------------------------------------------
	//-----------------------Orthographic Camera-----------------------
	//-----------------------------------------------------------------

	OrthoCameraController::OrthoCameraController(float aspectRatio, float cameraSpeed, float acceleration, float damping)
	: m_aspectRatio(aspectRatio), m_cameraSpeed(cameraSpeed), m_acceleration(acceleration), m_damping(damping)
	{
		m_Camera = std::make_shared<OrthoCamera>(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);
	}

	void OrthoCameraController::onUpdate(DeltaTime dt)
	{
		recalculateCameraVectors();

		float cameraSpeed = m_cameraSpeed * (Input::isKeyPressed(TST_KEY_LEFT_CONTROL) ? 3.0f : 1.0f);

		TST_CORE_INFO("{0}, {1}", m_zoom, m_aspectRatio);
		glm::vec3 cameraDirection{ 0.0f };
		if (Input::isKeyPressed(TST_KEY_W))			 { cameraDirection += glm::vec3(m_CameraFront.x, 0.0f, m_CameraFront.z); }
		if (Input::isKeyPressed(TST_KEY_S))			 { cameraDirection -= glm::vec3(m_CameraFront.x, 0.0f, m_CameraFront.z); }
		if (Input::isKeyPressed(TST_KEY_A))			 { cameraDirection -= glm::vec3(m_CameraRight.x, 0.0f, m_CameraRight.z); }
		if (Input::isKeyPressed(TST_KEY_D))			 { cameraDirection += glm::vec3(m_CameraRight.x, 0.0f, m_CameraRight.z); }
		if (Input::isKeyPressed(TST_KEY_SPACE))		 { cameraDirection += glm::vec3(0.0f, 1.0f, 0.0f); }
		if (Input::isKeyPressed(TST_KEY_LEFT_SHIFT)) { cameraDirection -= glm::vec3(0.0f, 1.0f, 0.0f); }

		if (glm::length(cameraDirection) > std::numeric_limits<float>::epsilon()) { cameraDirection = glm::normalize(cameraDirection); }

		glm::vec3 startingVelocity = cameraDirection * cameraSpeed;
		m_cameraVelocity = glm::mix(m_cameraVelocity, startingVelocity, dt * m_acceleration);
		m_cameraVelocity *= std::exp(-m_damping * dt);
		m_cameraPosition += m_cameraVelocity * dt.getTime_s();

		m_Camera->setPosition(m_cameraPosition);
		m_Camera->setRotation(m_cameraRotation);
	}

	void OrthoCameraController::onEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				return onKeyPressedEvent(e);
			});
		dispatcher.dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& e)
			{
				return onKeyReleasedEvent(e);
			});
		dispatcher.dispatch<MouseMoveEvent>([this](MouseMoveEvent& e)
			{
				return onMouseMoveEvent(e);
			});
		dispatcher.dispatch<MouseScrollEvent>([this](MouseScrollEvent& e)
			{
				return onMouseScrollEvent(e);
			});
		dispatcher.dispatch<WindowResizedEvent>([this](WindowResizedEvent& e)
			{
				return onWindowResizedEvent(e);
			});
	}

	bool OrthoCameraController::onKeyPressedEvent(KeyPressedEvent& e)
	{

		return false;
	}

	bool OrthoCameraController::onKeyReleasedEvent(KeyReleasedEvent& e)
	{

		return false;
	}

	bool OrthoCameraController::onMouseMoveEvent(MouseMoveEvent& e)
	{ 
		static float lastX   = 0.0f;
		static float lastY   = 0.0f;
		static float mouseDx = 0.0f;
		static float mouseDy = 0.0f;

		mouseDx = static_cast<float>(e.getMouseX()) - lastX;
		mouseDy = static_cast<float>(e.getMouseY()) - lastY;
		lastX   = static_cast<float>(e.getMouseX());
		lastY	= static_cast<float>(e.getMouseY());

		mouseDx *= 0.1f;
		mouseDy *= 0.1f;

		if (!Application::uiMode)
		{
			m_cameraRotation.x -= glm::radians(mouseDy);
			m_cameraRotation.y += glm::radians(mouseDx);

			if (m_cameraRotation.x > glm::radians(89.0f)) { m_cameraRotation.x = glm::radians(89.0f); }
			if (m_cameraRotation.x < glm::radians(-89.0f)) { m_cameraRotation.x = glm::radians(-89.0f); }

			m_cameraRotation.y = glm::mod(m_cameraRotation.y, glm::two_pi<float>());
		}

		mouseDy = 0.0f;	
		mouseDx = 0.0f;

		return false;
	}

	bool OrthoCameraController::onMouseScrollEvent(MouseScrollEvent& e)
	{
		m_zoom -= static_cast<float>(e.getScrollY());



		m_Camera->recalculateProjectionMatrix(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);

		return false;
	}

	bool OrthoCameraController::onWindowResizedEvent(WindowResizedEvent& e)
	{
		m_aspectRatio = static_cast<float>(e.getWidth()) / static_cast<float>(e.getHeight());
		m_Camera->recalculateProjectionMatrix(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);

		return false;
	}


	void OrthoCameraController::recalculateCameraVectors()
	{
		float cosPitch = glm::cos(m_cameraRotation.x);
		float sinPitch = glm::sin(m_cameraRotation.x);
		float cosYaw = glm::cos(m_cameraRotation.y);
		float sinYaw = glm::sin(m_cameraRotation.y);

		m_CameraFront = glm::normalize(glm::vec3(
			cosYaw * cosPitch,
			sinPitch,
			sinYaw * cosPitch
		));

		m_CameraRight = glm::normalize(glm::cross(m_CameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));

		m_CameraUp = glm::normalize(glm::cross(m_CameraRight, m_CameraFront));
	}

}
