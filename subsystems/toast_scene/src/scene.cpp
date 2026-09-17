#include "toast_scene/scene.hpp"

namespace toaster::scene
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	auto Scene::createEntity() -> entt::entity
	{
		return m_registry.create();
	}

	auto Scene::destroyEntity(entt::entity p_entity) -> void
	{
		m_registry.destroy(p_entity);
	}
}
