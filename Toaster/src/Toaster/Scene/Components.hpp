#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "glm/gtx/component_wise.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Toaster/Renderer/Camera.hpp"
#include "Toaster/Renderer/Light.hpp"
#include "Toaster/Renderer/MaterialSystem.hpp"
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

		glm::mat4 matrix() const
		{
			glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
			glm::mat4 rotationMatrix = glm::toMat4(glm::quat(rotation));
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

			return translationMatrix * rotationMatrix * scaleMatrix;
		}

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

	struct CameraComponent
	{
		CameraComponent() = default;
		SceneCamera camera;
		bool active{ true };
		bool fixedAspect{ false };
	};

	struct MaterialSlot
	{
		MaterialSlot() = default;
		MaterialSlot(MaterialID material_id) : material(material_id) {}
		MaterialSlot(MaterialInstanceID instance_id) : materialInstance(instance_id) {}

		MaterialID material{ TST_INVALID_MATERIAL };
		MaterialInstanceID materialInstance{ TST_INVALID_MATERIAL_INSTANCE };

		void setMaterial(MaterialID material_id) { material = material_id; materialInstance = TST_INVALID_MATERIAL_INSTANCE; }
		void setMaterialInstance(MaterialInstanceID instance_id) { materialInstance = instance_id; material = TST_INVALID_MATERIAL; }

		RefPtr<Material> getEffectiveMaterial() const
		{
			if (materialInstance != TST_INVALID_MATERIAL_INSTANCE) {
				return MaterialSystem::getMaterialInstance(materialInstance);
			}

			if (material != TST_INVALID_MATERIAL) {
				return MaterialSystem::getMaterial(material);
			}

			return nullptr;
		}

	};


	// Advanced render settings component
	struct RenderSettingsComponent
	{
		RenderSettingsComponent() = default;

		// Rendering flags
		bool visible = true;
		bool castShadows = true;
		bool receiveShadows = true;

		// Culling and LOD
		float lodBias = 1.0f;
		uint32_t renderLayer = 0;

		// Advanced settings
		bool wireframe = false;
		glm::vec4 tintColour = glm::vec4(1.0f);

		// Material animation support
		float animationTime = 0.0f;
		bool enableAnimation = false;
	};

	struct MeshRendererComponent
	{
		MeshRendererComponent() = default;
		MeshRendererComponent(const RefPtr<Mesh> &mesh) : mesh(mesh)
		{
			if (mesh) { initMaterialSlots(); }
		}

		RefPtr<Mesh> mesh{ nullptr };
		std::vector<MaterialSlot> materialSlots;

		bool castShadows = true;
		bool receiveShadows = true;
		bool showBoundingBox = false;
		bool showWireframe = false;
		uint32_t renderLayer = 0;
		uint32_t lightMapIndex = 0;
		glm::vec4 lightMapScaleOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);

	private:
		void initMaterialSlots()
		{
			const auto& submeshes = mesh->getSubMeshes();
			materialSlots.resize(submeshes.size());

			const auto& meshMaterials = mesh->getMaterialIDs();
			for (size_t i = 0; i < submeshes.size(); i++)
			{
				auto meshMaterial = mesh->getMaterial(meshMaterials[i]);
				if (meshMaterial)
				{
					auto globMatId = MaterialSystem::createMaterial(meshMaterial->getName());
					materialSlots[i].setMaterial(globMatId);
				} else
				{
					auto defaultId = MaterialSystem::getMaterialID("Default");
					materialSlots[i].setMaterial(defaultId);
				}
			}
		}
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
