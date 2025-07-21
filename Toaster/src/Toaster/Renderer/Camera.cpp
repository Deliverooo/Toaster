#include "tstpch.h"
#include "Camera.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace tst
{

	Camera::Camera(float fov, float aspect, float zNear, float zFar) : fov(fov), zNear(zNear), zFar(zFar), m_aspect(aspect)
	{
		position = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f, 6.283185f, 0.0f);
		calcViewMatrix();
		m_projectionMatrix = glm::perspective(fov, aspect, zNear, zFar);
		setCameraMode(CameraMode::PERSPECTIVE);
	}

	Camera::Camera(float left, float right, float bottom, float top, float zNear, float zFar) : leftPlane(left), rightPlane(right), bottomPlane(bottom), topPlane(top), zNear(zNear), zFar(zFar)
	{
		position = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f, 6.283185f, 0.0f);
		calcViewMatrix();
		m_projectionMatrix = glm::ortho(left * m_aspect, right * m_aspect, bottom, top, zNear, zFar);
		setCameraMode(CameraMode::ORTHOGRAPHIC);
	}

	void Camera::updateCameraMode(CameraMode mode)
	{
		if (mode == CameraMode::PERSPECTIVE)
		{
			m_projectionMatrix = glm::perspective(fov, m_aspect, zNear, zFar);
		}
		else
		{
			m_projectionMatrix = glm::ortho(leftPlane * m_aspect, rightPlane * m_aspect, bottomPlane, topPlane, zNear, zFar);
		}
		m_cameraMode = mode;
	}

	void Camera::setAspect(float aspect)
	{
		m_aspect = aspect;
		updateCameraMode(m_cameraMode);
	}

	const glm::vec3 Camera::front() const
	{
		float cosPitch = glm::cos(rotation.x);
		float sinPitch = glm::sin(rotation.x);
		float cosYaw = glm::cos(rotation.y);
		float sinYaw = glm::sin(rotation.y);

		return glm::normalize(glm::vec3(
			cosYaw * cosPitch,
			sinPitch,
			sinYaw * cosPitch
		));
	}

	const glm::vec3 Camera::right() const
	{
		glm::vec3 f = front();
		return glm::normalize(glm::cross(f, glm::vec3(0.0f, 1.0f, 0.0f)));
	}

	const glm::vec3 Camera::up() const
	{
		return glm::normalize(glm::cross(right(), front()));
	}

	void Camera::calcViewMatrix()
	{
		m_viewMatrix = glm::lookAt(position, position + front(), { 0.0f, 1.0f, 0.0f });
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

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 10.0f);
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

		m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 10.0f);
	}
}
