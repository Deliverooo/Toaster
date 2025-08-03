#include "tstpch.h"
#include "Scene.hpp"

#include "Entity.hpp"
#include "Components.hpp"
#include "ScriptableEntity.hpp"

#include "Toaster/Renderer/Renderer2D.hpp"
#include "Toaster/Renderer/Renderer3D.hpp"

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Toaster/Renderer/MeshRenderer.hpp"


namespace tst
{

	struct Transform
	{
		Transform(const glm::mat4& transform) : matrix(transform) {}

		operator glm::mat4& () { return matrix; }
		operator const glm::mat4& () const { return matrix; }

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

			// 2D Primitive Rendering
			Renderer2D::begin(mainCamera->getProjection(), *cameraView);
			auto group2d = m_registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);
			for (auto entity : group2d)
			{
				const auto& [spriteRenderer, transform] = group2d.get<SpriteRendererComponent, TransformComponent>(entity);

				if (spriteRenderer.texture)
				{
					Renderer2D::drawQuad(transform.matrix(), spriteRenderer.texture, 1.0f, spriteRenderer.colour);
				} else
				{
					Renderer2D::drawQuad(transform.matrix(), spriteRenderer.colour);
				}
			}
			Renderer2D::end();



			// Mesh Rendering
			MeshRenderer::begin(mainCamera->getProjection(), *cameraView);
			auto meshGroup = m_registry.group<MeshRendererComponent>(entt::get<TransformComponent>);
			for (auto entity : meshGroup)
			{
				const auto& [meshRenderer, transform] = meshGroup.get<MeshRendererComponent, TransformComponent>(entity);
				MeshRenderer::drawMesh(meshRenderer.mesh, transform.matrix());
			}
			MeshRenderer::end();



		}
	}

	void Scene::onViewportResize(uint32_t width, uint32_t height)
	{

		m_ViewportWidth = width;
		m_ViewportHeight = height;

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


	Entity Scene::createEntity(const std::string& name)
	{
		Entity entity = { m_registry.create(), this };
		entity.addComponent<TransformComponent>();
		auto& tag = entity.addComponent<TagComponent>();

		tag.name = (name.empty()) ? "Entity" : name;

		return entity;
	}

	void Scene::removeEntity(const Entity& entity)
	{
		m_registry.destroy(entity);
	}

	template<typename T>
	void Scene::onComponentAdd(Entity* entity, T& component)
	{

	}

	template<>
	void Scene::onComponentAdd<TagComponent>(Entity* entity, TagComponent& component)
	{

	}

	template<>
	void Scene::onComponentAdd<TransformComponent>(Entity* entity, TransformComponent& component)
	{

	}

	template<>
	void Scene::onComponentAdd<CameraComponent>(Entity* entity, CameraComponent& component)
	{
		component.camera.setViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::onComponentAdd<SpriteRendererComponent>(Entity* entity, SpriteRendererComponent& component)
	{

	}

	template<>
	void Scene::onComponentAdd<MeshRendererComponent>(Entity* entity, MeshRendererComponent& component)
	{

	}

	template<>
	void Scene::onComponentAdd<NativeScriptComponent>(Entity* entity, NativeScriptComponent& component)
	{

	}

	template<>
	void Scene::onComponentAdd<LightComponent>(Entity* entity, LightComponent& component)
	{

	}
}