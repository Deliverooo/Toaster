#pragma once

#include "toast_scene.hpp"

#include <entt/entt.hpp>

namespace toaster::scene
{
	class TST_SCENE_API Scene
	{
	public:
		Scene();
		~Scene();

		auto createEntity() -> entt::entity;
		auto destroyEntity(entt::entity p_entity) -> void;

		template<typename TComp, typename... TArgs>
		auto addComponent(entt::entity p_entity, TArgs &&... p_args) -> TComp &
		{
			return m_registry.emplace<TComp>(p_entity, std::forward<TArgs>(p_args)...);
		}

		template<typename TComp>
		auto removeComponent(entt::entity p_entity) -> void
		{
			m_registry.remove<TComp>(p_entity);
		}

		auto getRegistry() -> entt::registry & { return m_registry; }
		auto getRegistry() const -> const entt::registry & { return m_registry; }

	private:
		entt::registry m_registry;
	};
}
