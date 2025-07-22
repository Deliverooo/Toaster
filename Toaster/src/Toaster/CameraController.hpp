#pragma once
#include "Core/Time.hpp"
#include "Events/ApplicationEvent.hpp"
#include "Events/Event.hpp"
#include "Events/KeyEvent.hpp"
#include "Events/MouseEvent.hpp"
#include "glm/gtc/constants.hpp"
#include "Renderer/Camera.hpp"

namespace tst
{

	class PerspectiveCameraController
	{
	public:

		PerspectiveCameraController(float fov, float aspectRatio, float cameraSpeed = 5.0f, float acceleration = 5.0f, float damping = 5.0f);

		void onUpdate(DeltaTime dt);
		void onEvent(Event &e);

		const RefPtr<PerspectiveCamera>& getCamera() const { return m_Camera; }

		const glm::vec3& getVelocity() const { return m_cameraVelocity; }
		const glm::vec3& getPosition() const { return m_cameraPosition; }
		const glm::vec3& getRotation() const { return m_cameraRotation; }

		const glm::vec3& getFrontVector() const { return m_CameraFront; }
		const glm::vec3& getRightVector() const { return m_CameraRight; }
		const glm::vec3& getUpVector()	  const { return m_CameraUp; }


	private:

		bool onKeyPressedEvent(KeyPressedEvent& e);
		bool onKeyReleasedEvent(KeyReleasedEvent& e);
		bool onMouseMoveEvent(MouseMoveEvent& e);
		bool onMouseScrollEvent(MouseScrollEvent& e);
		bool onWindowResizedEvent(WindowResizedEvent& e);

		void recalculateCameraVectors();

		float m_aspectRatio{1.77f};
		float m_fov;

		float m_cameraSpeed { 1.0f };
		float m_acceleration{ 1.0f };
		float m_damping		{ 0.1f };

		glm::vec3 m_cameraVelocity { 0.0f };

		glm::vec3 m_cameraPosition { 0.0f };
		glm::vec3 m_cameraRotation { 0.0f };
		glm::mat4 m_cameraTransform{ 1.0f };

		glm::vec3 m_CameraFront{ 0.0f, 0.0f, -1.0f };
		glm::vec3 m_CameraRight{ 1.0f, 0.0f, 0.0f  };
		glm::vec3 m_CameraUp   { 0.0f, 1.0f, 0.0f  };
		
		RefPtr<PerspectiveCamera> m_Camera;
	};


	class OrthoCameraController
	{
	public:

		OrthoCameraController(float aspectRatio, float cameraSpeed = 5.0f, float acceleration = 5.0f, float damping = 5.0f);

		void onUpdate(DeltaTime dt);
		void onEvent(Event& e);

		const RefPtr<OrthoCamera>& getCamera() const { return m_Camera; }

	private:

		bool onKeyPressedEvent(KeyPressedEvent& e);
		bool onKeyReleasedEvent(KeyReleasedEvent& e);
		bool onMouseMoveEvent(MouseMoveEvent& e);
		bool onMouseScrollEvent(MouseScrollEvent& e);
		bool onWindowResizedEvent(WindowResizedEvent& e);

		void recalculateCameraVectors();

		float m_aspectRatio{1.77f};

		float m_zoom{1.0f};

		float m_cameraSpeed {1.0f};
		float m_acceleration{1.0f};
		float m_damping		{0.1f};

		glm::vec3 m_cameraVelocity { 0.0f };

		glm::vec3 m_cameraPosition { 0.0f };
		glm::vec3 m_cameraRotation { 0.0f, 0.0f, 0.0f };
		glm::mat4 m_cameraTransform{ 1.0f };

		glm::vec3 m_CameraFront{ 0.0f, 0.0f, -1.0f };
		glm::vec3 m_CameraRight{ 1.0f, 0.0f, 0.0f  };
		glm::vec3 m_CameraUp   { 0.0f, 1.0f, 0.0f  };

		RefPtr<OrthoCamera> m_Camera;
	};



	class OrthoCamera2DController
	{
	public:

		OrthoCamera2DController(float aspectRatio);

		void onUpdate(DeltaTime dt);
		void onEvent(Event& e);

		const RefPtr<OrthoCamera2D>& getCamera() const { return m_Camera; }

		const glm::vec2& getVelocity() const { return m_cameraVelocity; }
		const glm::vec2& getPosition() const { return m_cameraPosition; }
		const float& getRotation() const	 { return m_cameraRotation; }


	private:

		bool onKeyPressedEvent(KeyPressedEvent& e);
		bool onKeyReleasedEvent(KeyReleasedEvent& e);
		bool onMouseMoveEvent(MouseMoveEvent& e);
		bool onMouseScrollEvent(MouseScrollEvent& e);
		bool onWindowResizedEvent(WindowResizedEvent& e);

		float m_aspectRatio{ 1.77f };

		float m_zoom{ 1.0f };

		float m_cameraSpeed{ 3.0f };
		float m_cameraRotationSpeed{ 180.0f };
		float m_acceleration{ 10.0f };
		float m_damping{ 1.0f };

		glm::vec2 m_cameraVelocity{ 0.0f };

		glm::vec3 m_cameraPosition{ 0.0f };
		float m_cameraRotation{ 0.0f };

		glm::mat4 m_cameraTransform{ 1.0f };


		RefPtr<OrthoCamera2D> m_Camera;
	};

}
