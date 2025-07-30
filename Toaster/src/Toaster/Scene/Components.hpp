#pragma once

#include <glm/glm.hpp>

#include "Toaster/Renderer/Camera.hpp"
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
		bool main{ true };
		bool fixedAspect{ false };
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
