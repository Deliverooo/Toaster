#include "tstpch.h"
#include "Camera.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Toaster/Core/Input.hpp"
#include "Toaster/Core/KeyCodes.hpp"
#include "Toaster/Core/MouseCodes.hpp"
#include "Toaster/Events/KeyEvent.hpp"

namespace tst
{

	SceneCamera::SceneCamera() : Camera()
	{
		recalculateProjectionMatrix();
	}

	void SceneCamera::setOrtho(float size, float zNear, float zFar)
	{
		m_OrthoSize = size;
		m_OrthoNear = zNear;
		m_OrthoFar	= zFar;

		recalculateProjectionMatrix();
	}
	void SceneCamera::setPerspective(float fov, float zNear, float zFar)
	{
		m_PersFov  = fov;
		m_PersNear = zNear;
		m_PersFar  = zFar;

		recalculateProjectionMatrix();
	}

	void SceneCamera::setViewportSize(uint32_t width, uint32_t height)
	{
		uint32_t safeWidth	= std::max(width,  static_cast<uint32_t>(1));
		uint32_t safeHeight = std::max(height, static_cast<uint32_t>(1));

		m_Aspect = static_cast<float>(safeWidth) / static_cast<float>(safeHeight);
		recalculateProjectionMatrix();
	}

	void SceneCamera::recalculateProjectionMatrix()
	{
		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			m_projectionMatrix = glm::perspective(glm::radians(m_PersFov), m_Aspect, m_PersNear, m_PersFar);
			break;
		case ProjectionType::Orthographic:
			m_projectionMatrix = glm::ortho(-m_OrthoSize * m_Aspect, m_OrthoSize * m_Aspect, -m_OrthoSize, m_OrthoSize, m_OrthoNear, m_OrthoFar);
			break;

		}
	}


	EditorCamera::EditorCamera(float fov, float aspect, float zNear, float zFar) : Camera(glm::perspective(glm::radians(fov), aspect, zNear, zFar)), 
		m_fov(fov), m_aspect(aspect), m_zNear(zNear), m_zFar(zFar)
	{
		recalculateViewMatrix();
	}

	void EditorCamera::onUpdate(DeltaTime dt)
	{
		static glm::vec2 lastMousePos = { Input::getMouseX(), Input::getMouseY() };
		static glm::vec2 delta = { 0.0f, 0.0f };



		//bool shiftDown = Input::isKeyPressed(Key::LeftShift);
		//bool ctrlDown = Input::isKeyPressed(Key::LeftControl);
		bool altDown = Input::isKeyPressed(Key::LeftAlt);

		if (altDown)
		{
			glm::vec2 mousePos = { Input::getMousePos().first, Input::getMousePos().second };
			delta = (mousePos - lastMousePos) * dt.getTime_s();
			lastMousePos = mousePos;

			if (Input::isMouseButtonPressed(MouseButton::Left))		{ zoomMouse(delta.y); }
			else if (Input::isMouseButtonPressed(MouseButton::Right))    { orbitMouse(delta); }
			else if (Input::isMouseButtonPressed(MouseButton::Middle))   { panMouse(delta); }


		}

		recalculateViewMatrix();
	}

	void EditorCamera::panMouse(const glm::vec2& mouseDelta)
	{
		glm::vec2 panSpeed = calcPanSpeed();
		m_focalPoint += -getCameraRight() * mouseDelta.x * m_distance * panSpeed.x;
		m_focalPoint += getCameraUp() * mouseDelta.y * m_distance * panSpeed.y;
	}

	void EditorCamera::orbitMouse(const glm::vec2& mouseDelta)
	{
		float yaw = getCameraUp().y < 0.0f ? -1.0f : 1.0f;
		m_rotation.y += yaw * mouseDelta.x * 0.4f;
		m_rotation.x += mouseDelta.y * 0.4f;
	}

	void EditorCamera::zoomMouse(const float zoomDelta)
	{
		m_distance -= zoomDelta * calcZoomSpeed();
		if (m_distance < 1.0f)
		{
			m_focalPoint += getCameraForward();
			m_distance = 1.0f;
		}
	}

	void EditorCamera::onEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<MouseScrollEvent>([this](MouseScrollEvent &event)
		{
			return onMouseScrolled(event);
		});

		dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& event)
		{
			return onKeyPressedEvent(event);
		});
	}

	bool EditorCamera::onKeyPressedEvent(KeyPressedEvent& e)
	{
		if (e.getKeycode() == Key::H)
		{
			m_position = { 0.0f, 0.0f, 0.0f };
			m_focalPoint = { 0.0f, 0.0f, 0.0f };
			m_distance = 10.0f;
		}

		return false;
	}


	bool EditorCamera::onMouseScrolled(MouseScrollEvent& e)
	{
		float delta = e.getScrollY() * 0.1f;
		zoomMouse(delta);
		recalculateViewMatrix();
		return false;
	}

	void EditorCamera::recalculateViewMatrix()
	{
		m_position = calcPosition();
		m_viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(getRotation()));
	}

	void EditorCamera::recalculateProjectionMatrix()
	{
		m_aspect = m_ViewportWidth / m_ViewportHeight;
		m_projectionMatrix = glm::perspective(glm::radians(m_fov), m_aspect, m_zNear, m_zFar);
	}

	glm::vec3 EditorCamera::getCameraForward() const
	{
		return glm::rotate(getRotation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::getCameraRight() const
	{
		return glm::rotate(getRotation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::getCameraUp() const
	{
		return glm::rotate(getRotation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::calcPosition() const
	{
		return m_focalPoint - getCameraForward() * m_distance;
	}

	glm::vec2 EditorCamera::calcPanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}
	float EditorCamera::calcZoomSpeed() const
	{
		float distance = m_distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}







	//----------------------------------------------------------------
	//-----------------------Perspective Camera-----------------------
	//----------------------------------------------------------------

	PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float zNear, float zFar) : m_position(0.0f), m_viewMatrix(1.0f), m_rotation(0.0f)
	{
		m_projectionMatrix = glm::perspective(fov, aspect, zNear, zFar);
	}

	void PerspectiveCamera::recalculateViewMatrix()
	{
		TST_PROFILE_FN();

		float cosPitch = glm::cos(m_rotation.x);
		float sinPitch = glm::sin(m_rotation.x);
		float cosYaw   = glm::cos(m_rotation.y);
		float sinYaw   = glm::sin(m_rotation.y);

		glm::vec3 front =  glm::normalize(glm::vec3(
			cosYaw * cosPitch,
			sinPitch,
			sinYaw * cosPitch
		));

		m_viewMatrix = glm::lookAt(m_position, m_position + front, { 0.0f, 1.0f, 0.0f });
	}

	void PerspectiveCamera::recalculateProjectionMatrix(float fov, float aspect, float zNear, float zFar)
	{
		TST_PROFILE_FN();

		m_projectionMatrix = glm::perspective(fov, aspect, zNear, zFar);
	}


	//-----------------------------------------------------------------
	//-----------------------Orthographic Camera-----------------------
	//-----------------------------------------------------------------

	OrthoCamera::OrthoCamera(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar)
	{
		m_position = { 0.0f, 0.0f, 0.0f};
		m_rotation = { 0.0f, 0.0f, 0.0f };

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 10.0f);
		m_viewMatrix = glm::mat4(1.0f);
	}

	void OrthoCamera::recalculateViewMatrix()
	{
		TST_PROFILE_FN();

		float cosPitch = glm::cos(m_rotation.x);
		float sinPitch = glm::sin(m_rotation.x);
		float cosYaw = glm::cos(m_rotation.y);
		float sinYaw = glm::sin(m_rotation.y);

		glm::vec3 front = glm::normalize(glm::vec3(
			cosYaw * cosPitch,
			sinPitch,
			sinYaw * cosPitch
		));

		m_viewMatrix = glm::lookAt(m_position, m_position + front, { 0.0f, 1.0f, 0.0f });
	}

	void OrthoCamera::recalculateProjectionMatrix(const float left, const float right, const float bottom, const float top)
	{
		TST_PROFILE_FN();

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 10.0f);
	}

	//--------------------------------------------------------------------
	//-----------------------Orthographic 2D Camera-----------------------
	//--------------------------------------------------------------------

	OrthoCamera2D::OrthoCamera2D(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar)
	{
		TST_PROFILE_FN();

		m_position = { 0.0f, 0.0f, 0.0f };
		m_rotation = { 0.0f };

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
	}

	void OrthoCamera2D::recalculateViewMatrix()
	{
		TST_PROFILE_FN();

		glm::mat4 trans = glm::translate(glm::mat4(1.0f), m_position);
		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), {0.0f, 0.0f, 1.0f});

		m_viewMatrix = glm::inverse(trans * rot);
	}

	void OrthoCamera2D::recalculateProjectionMatrix(const float left, const float right, const float bottom, const float top)
	{
		TST_PROFILE_FN();

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
	}
}
