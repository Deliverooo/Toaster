#pragma once

#include <glm/glm.hpp>

#include "Toaster/Renderer/Camera.hpp"
#include "Toaster/Renderer/Light.hpp"
#include "Toaster/Renderer/Mesh.hpp"
#include "Toaster/Util/MathUtil.hpp"

namespace tst
{
	struct TagComponent
	{
		TagComponent() = default;
		TagComponent(const std::string& name) : name(name) {}


		std::string name{};
	};

	struct TransformComponent
	{
		TransformComponent() = default;
		TransformComponent(const glm::vec3& translation) : translation(translation) {}

		glm::mat4 matrix() const { return transformationMat(translation, rotation, scale); }

		glm::vec3 translation{ 0.0f };
		glm::vec3 rotation{ 0.0f };
		glm::vec3 scale{ 1.0f };
	};

	struct SpriteRendererComponent
	{
		SpriteRendererComponent() = default;
		SpriteRendererComponent(const glm::vec4& colour) : colour(colour) {}

		glm::vec4 colour = { 1.0f, 0.0f, 0.862f, 1.0f };
		RefPtr<Texture2D> texture = nullptr;
	};

	struct MaterialComponent
	{
		MaterialComponent() = default;
		MaterialComponent(const glm::vec4& colour) : colour(colour) {}

		glm::vec4 colour{ 1.0f, 0.0f, 0.862f, 1.0f };
	};


	struct CameraComponent
	{
		CameraComponent() = default;


		SceneCamera camera;
		bool active{ true };
		bool fixedAspect{ false };
	};

	struct MeshRendererComponent
	{
		MeshRendererComponent() = default;
		MeshRendererComponent(const glm::vec4& colour) : colour(colour) {}
		MeshRendererComponent(RefPtr<Mesh> mesh, const glm::vec4& colour = { 1.0f, 1.0f, 1.0f, 1.0f })
			: mesh(mesh), colour(colour) {
		}

		RefPtr<Mesh> mesh = nullptr;
		glm::vec4 colour = { 1.0f, 0.0f, 0.862f, 1.0f };
	};

	struct LightComponent
	{
		LightComponent() = default;
		LightComponent(Light::Type type) : light(type) {}

		Light light{};
		bool enabled{ true };

	};

	using TstBehaviorFn = std::function<void()>;

	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity *instance = nullptr;

		ScriptableEntity* (*instantiateScript)();
		void (*destroyScript)(NativeScriptComponent*);

		template<typename T>
		void bind()
		{
			instantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };

			destroyScript = [](NativeScriptComponent* script)
			{
				TST_ASSERT(script->instance != nullptr, "NativeScriptComponent::destructFn -> attempted to delete instance reference to nullptr!");
				delete script->instance;
				script->instance = nullptr;
			};


		}
	};



}
