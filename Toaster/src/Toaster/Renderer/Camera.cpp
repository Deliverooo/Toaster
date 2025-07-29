#include "tstpch.h"
#include "Camera.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace tst
{

	//constexpr glm::mat4 iso(const float size, const float zNear, const float zFar)
	//{
	//	glm::mat4 projection = glm::ortho(-size, size, -size, size, zNear, zFar);
	//	projection[0][0] = 1.0f / (size * 2.0f);
	//	projection[1][1] = 1.0f / (size * 2.0f);
	//	projection[2][2] = 1.0f / (zFar - zNear);
	//	projection[3][3] = 1.0f;
	//	return projection;
	//}

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
