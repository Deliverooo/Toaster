#pragma once
#include <glm/glm.hpp>

namespace tst
{

	class TST_API Camera
	{
	public:

		enum class CameraMode
		{
			PERSPECTIVE, ORTHOGRAPHIC, ISOMETRIC,
		};


		Camera(float fov, float aspect, float zNear = 0.1f, float zFar = 100.0f);
		Camera(float left, float right, float bottom, float top, float zNear = 0.1f, float zFar = 100.0f);

		~Camera() {}

		void setPosition(const glm::vec3& newPosition) { position = newPosition; calcViewMatrix(); }
		void setRotation(const glm::vec3& newRotation) { rotation = newRotation; calcViewMatrix(); }
		void setCameraMode(CameraMode cameraMode) { m_cameraMode = cameraMode; }
		void setAspect(float aspect);

		void move(const glm::vec3& translation) { position += translation; }
		void rotate(const glm::vec3& rot) { rotation += rot; }


		void updateCameraMode(CameraMode mode);

		[[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		[[nodiscard]] const glm::mat4& getViewMatrix() { calcViewMatrix(); return m_viewMatrix; }

		[[nodiscard]] const glm::vec3& getPosition() const { return position; }
		[[nodiscard]] const glm::vec3& getRotation() const { return rotation; }

		[[nodiscard]] const glm::vec3 front() const;
		[[nodiscard]] const glm::vec3 right() const;
		[[nodiscard]] const glm::vec3 up()	  const;

		[[nodiscard]] const float& getYaw()		const { return yaw; }
		[[nodiscard]] const float& getPitch()	const { return pitch; }
		[[nodiscard]] const float& getRoll()	const { return roll; }

		[[nodiscard]] const CameraMode& getCameraMode() const { return m_cameraMode; }


		void setFov(const float fovy) { fov = fovy; }
		const float& getFov() const { return fov; }

		float fov{90.0f};
		float zNear;
		float zFar;


		float leftPlane{-1.0f};
		float rightPlane{1.0f};
		float bottomPlane{-1.0f};
		float topPlane{1.0f};

		glm::vec3 position;

		union
		{
			glm::vec3 rotation;
			struct
			{
				float yaw;
				float pitch;
				float roll;
			};
		};


	protected:

		void calcViewMatrix();

		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;

		float m_aspect{ 16.0f / 9.0f };

		CameraMode m_cameraMode;
	};

	//----------------------------------------------------------------
	//-----------------------Perspective Camera-----------------------
	//----------------------------------------------------------------

	class TST_API PerspectiveCamera
	{
	public:
		PerspectiveCamera(float fov, float aspect, float zNear = 0.1f, float zFar = 100.0f);

		[[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		[[nodiscard]] const glm::mat4& getViewMatrix() { recalculateViewMatrix(); return m_viewMatrix; }

		void setPosition(const glm::vec3& newPosition) { m_position = newPosition; recalculateViewMatrix(); }
		void setRotation(const glm::vec3& newRotation) { m_rotation = newRotation; recalculateViewMatrix(); }


	private:

		void recalculateViewMatrix();
		void recalculateProjectionMatrix(float fov, float aspect, float zNear, float zFar);

		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;

		glm::vec3 m_position;

		union
		{
			glm::vec3 m_rotation;
			struct
			{
				float m_yaw;
				float m_pitch;
				float m_roll;
			};
		};

		friend class PerspectiveCameraController;
	};

	//-----------------------------------------------------------------
	//-----------------------Orthographic Camera-----------------------
	//-----------------------------------------------------------------

	class TST_API OrthoCamera
	{
	public:
		OrthoCamera(const float left, const float right, const float bottom, const float top, const float zNear = -1.0f, const float zFar = 10.0f);

		[[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		[[nodiscard]] const glm::mat4& getViewMatrix() { recalculateViewMatrix(); return m_viewMatrix; }


		void setPosition(const glm::vec3& newPosition) { m_position = newPosition; recalculateViewMatrix(); }
		void setRotation(const glm::vec3& newRotation) { m_rotation = newRotation; recalculateViewMatrix(); }


	private:

		void recalculateViewMatrix();
		void recalculateProjectionMatrix(const float left, const float right, const float bottom, const float top);

		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;

		glm::vec3 m_position;

		union
		{
			glm::vec3 m_rotation;
			struct
			{
				float m_yaw;
				float m_pitch;
				float m_roll;
			};
		};

		friend class OrthoCameraController;
	};


	//--------------------------------------------------------------------
	//-----------------------Orthographic 2D Camera-----------------------
	//--------------------------------------------------------------------

	class TST_API OrthoCamera2D
	{
	public:
		OrthoCamera2D(const float left, const float right, const float bottom, const float top, const float zNear = -1.0f, const float zFar = 10.0f);

		[[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		[[nodiscard]] const glm::mat4& getViewMatrix() { recalculateViewMatrix(); return m_viewMatrix; }


		void setPosition(const glm::vec3& newPosition) { m_position = newPosition; recalculateViewMatrix(); }
		void setPosition(const glm::vec2& newPosition) { m_position = glm::vec3(newPosition.x, newPosition.y, 0.0f); recalculateViewMatrix(); }
		void setRotation(const float newRotation) { m_rotation = newRotation; recalculateViewMatrix(); }

		void recalculateProjectionMatrix(const float left, const float right, const float bottom, const float top);

	private:

		void recalculateViewMatrix();

		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix{ 1.0f };

		glm::vec3 m_position;
		float m_rotation;

		friend class OrthoCamera2DController;
	};
}


