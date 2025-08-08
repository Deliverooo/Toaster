#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Toaster/Core/Time.hpp"
#include "Toaster/Events/Event.hpp"
#include "Toaster/Events/MouseEvent.hpp"

namespace tst
{
	class KeyPressedEvent;
}

namespace tst
{
	// The base class for all cameras in the engine.
	// It provides a projection matrix that can be used for rendering.
	// This class is not meant to be instantiated directly, but rather to be inherited by other camera classes.
	class TST_API Camera
	{
	public:

		virtual ~Camera() = default;

		Camera() = default;
		Camera(const glm::mat4& projection) : m_projectionMatrix(projection) {}

		[[nodiscard]] const glm::mat4& getProjection() const { return m_projectionMatrix; }

	protected:
		glm::mat4 m_projectionMatrix{ 1.0f };
	};


	class TST_API SceneCamera : public Camera
	{
	public:

		virtual ~SceneCamera() override = default;
		SceneCamera();

		void setOrtho(float size, float zNear, float zFar);
		void setPerspective(float fov, float zNear, float zFar);


		void setViewportSize(uint32_t width, uint32_t height);

		
		float getOrthoSize() const { return m_OrthoSize; }
		void setOrthoSize(float size) { m_OrthoSize = size; recalculateProjectionMatrix(); }

		float getOrthoNear() const { return m_OrthoNear; }
		void setOrthoNear(float zNear) { m_OrthoNear = zNear; recalculateProjectionMatrix(); }

		float getOrthoFar() const { return m_OrthoFar; }
		void setOrthoFar(float zFar) { m_OrthoFar = zFar; recalculateProjectionMatrix(); }



		float getPerspectiveFov() const { return m_PersFov; }
		void setPerspectiveFov(float fov) { m_PersFov = fov; recalculateProjectionMatrix(); }

		float getPerspectiveNear() const { return m_PersNear; }
		void setPerspectiveNear(float zNear) { m_PersNear = zNear; recalculateProjectionMatrix(); }

		float getPerspectiveFar() const { return m_PersFar; }
		void setPerspectiveFar(float zFar) { m_PersFar = zFar; recalculateProjectionMatrix(); }


		enum class ProjectionType : int
		{
			Perspective = 0,
			Orthographic = 1,
		};

		ProjectionType getProjectionType() const { return m_ProjectionType; }
		void setProjectionType(ProjectionType type)
		{
			m_ProjectionType = type;
			recalculateProjectionMatrix();
		}

	private:

		void recalculateProjectionMatrix();

		ProjectionType m_ProjectionType{ ProjectionType::Perspective };

		float m_Aspect{ 1.77f };

		float m_OrthoSize{ 10.0f };
		float m_OrthoNear{ -1.0f };
		float m_OrthoFar { 10.0f };

		float m_PersFov {  90.0f };
		float m_PersNear{  0.1f  };
		float m_PersFar { 100.0f };
	};

	class EditorCamera : public Camera
	{
	public:
		virtual ~EditorCamera() override = default;
		EditorCamera(float fov = 45.0f, float aspect = 1.77f, float zNear = 0.001f, float zFar = 2000.0f);

		void setViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = static_cast<float>(width); m_ViewportHeight = static_cast<float>(height);	recalculateProjectionMatrix(); }

		void setPosition(const glm::vec3& position) { m_position = position; recalculateViewMatrix(); }
		void setRotation(const glm::vec3& rotation) { m_rotation = rotation; recalculateViewMatrix(); }

		const glm::vec3& getPosition() const { return m_position; }
		glm::quat getRotation() const { return glm::quat(glm::vec3(-m_rotation.x, -m_rotation.y, -m_rotation.z)); }
		const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
		const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }

		glm::vec3 getCameraForward() const;
		glm::vec3 getCameraRight() const;
		glm::vec3 getCameraUp() const;

		void onUpdate(DeltaTime dt);
		void onEvent(Event& e);

	private:
		void recalculateViewMatrix();
		void recalculateProjectionMatrix();
		glm::vec3 calcPosition() const;

		bool onMouseScrolled(MouseScrollEvent& e);
		bool onKeyPressedEvent(KeyPressedEvent& e);

		void panMouse(const glm::vec2& mouseDelta);
		void orbitMouse(const glm::vec2& mouseDelta);
		void zoomMouse(const float zoomDelta);

		glm::vec2 calcPanSpeed() const;
		float calcZoomSpeed() const;

		glm::mat4 m_viewMatrix;

		union
		{
			glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f };
			struct
			{
				float m_pitch;
				float m_yaw;
				float m_roll;
			};
		};

		glm::vec3 m_position  { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_focalPoint{ 0.0f, 0.0f, 0.0f };

		// The distance of the camera from the focal point
		float m_distance{ 10.0f };

		float m_fov{ 45.0f };
		float m_aspect{ 1.77f };
		float m_zNear{ 0.1f };
		float m_zFar{ 1000.0f };

		float m_ViewportWidth{ 1280.0f };
		float m_ViewportHeight{ 720.0f };
	};

	//-----------------------Perspective Camera-----------------------
	//----------------------------------------------------------------

	class TST_API PerspectiveCamera
	{
	public:
		PerspectiveCamera(float fov, float aspect, float zNear = 0.1f, float zFar = 10.0f);

		[[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
		[[nodiscard]] const glm::mat4& getViewMatrix() { recalculateViewMatrix(); return m_viewMatrix; }

		void setPosition(const glm::vec3& newPosition) { m_position = newPosition; recalculateViewMatrix(); }
		void setRotation(const glm::vec3& newRotation) { m_rotation = newRotation; recalculateViewMatrix(); }

		[[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
		[[nodiscard]] const glm::vec3& getRotation() const { return m_rotation; }

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

		[[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
		[[nodiscard]] const glm::vec3& getRotation() const { return m_rotation; }

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

		[[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
		[[nodiscard]] const float& getRotation() const { return m_rotation; }

	private:

		void recalculateViewMatrix();

		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix{ 1.0f };

		glm::vec3 m_position;
		float m_rotation;

		friend class OrthoCamera2DController;
	};
}


