#include "tstpch.h"
#include "Scene.hpp"

#include "Entity.hpp"
#include "Components.hpp"
#include "ScriptableEntity.hpp"

#include "Toaster/Renderer/Renderer2D.hpp"
#include "Toaster/Renderer/Renderer3D.hpp"

#include <glm/glm.hpp>

#include "Entity.hpp"


namespace tst
{

	struct Transform
	{
		Transform(const glm::mat4& transform) : matrix(transform) {}

		operator glm::mat4& (){return matrix; }
		operator const glm::mat4& () const {return matrix; }

		union
		{
			glm::mat4 transform;
			glm::mat4 matrix;
		};
	};

	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

		
	}

	
	void Scene::onUpdate(DeltaTime dt)
	{

		m_registry.view<NativeScriptComponent>().each([&](auto entity, auto& scriptComp)
		{
			if (scriptComp.instance == nullptr)
			{
				scriptComp.instance = scriptComp.instantiateScript();
				scriptComp.instance->m_entity = { entity, this };

				scriptComp.instance->onCreate();
			}

			scriptComp.instance->onUpdate(dt);
		});

		Camera* mainCamera = nullptr;
		glm::mat4* cameraView = nullptr;

		auto cameraGroup = m_registry.view<CameraComponent, TransformComponent>();
		for (auto entity : cameraGroup)
		{
			auto [camera, transform] = cameraGroup.get<CameraComponent, TransformComponent>(entity);

			if (camera.main)
			{
				glm::mat4 viewMatrix = transform.matrix();
				mainCamera = &camera.camera;
				cameraView = &viewMatrix;
				break;
			}
		}

		static float clock = 0.0f;
		clock += dt;

		if (mainCamera)
		{
			Renderer2D::begin(mainCamera->getProjection(), *cameraView);

			auto group = m_registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				const auto &[transform, spriteRenderer] = group.get<TransformComponent, SpriteRendererComponent>(entity);


				Renderer2D::drawQuad(transform.matrix(), spriteRenderer.colour);
			}

			Renderer2D::end();
		}
	}

	void Scene::onViewportResize(uint32_t width, uint32_t height)
	{

		auto cameraGroup = m_registry.view<CameraComponent>();
		for (auto entity : cameraGroup)
		{
			auto& camera = cameraGroup.get<CameraComponent>(entity);

			if (camera.fixedAspect)
			{
				continue;
			}

			camera.camera.setViewportSize(width, height);

		}
	}


	Entity Scene::createEntity(const std::string &name)
	{
		Entity entity = { m_registry.create(), this };
		entity.addComponent<TransformComponent>();
		auto& tag = entity.addComponent<TagComponent>();

		tag.name = (name.empty()) ? "Entity" : name;

		return entity;
	}
}
