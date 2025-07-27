#include "tstpch.h"
#include "Scene.hpp"

#include <glm/glm.hpp>

#include "Components.hpp"
#include "Toaster/Renderer/Renderer3D.hpp"

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

		auto group = m_registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);

		for (const auto entity : group)
		{
			auto [transform, spriteRenderer] = group.get<TransformComponent, SpriteRendererComponent>(entity);

			Renderer3D::drawQuad(transform, spriteRenderer.colour);
		}
	}

	Entity Scene::createEntity()
	{
		return { m_registry.create(), this };
	}


}
