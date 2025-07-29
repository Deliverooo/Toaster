#pragma once

#include "Toaster/Core/Time.hpp"
#include <entt.hpp>


namespace tst
{
	class Entity;

	class Scene
	{
	public:

		Scene();
		~Scene();

		void onUpdate(DeltaTime dt);
		void onViewportResize(uint32_t width, uint32_t height);

		Entity createEntity(const std::string& name);

		entt::registry &registry() { return m_registry; }

	private:

		entt::registry m_registry;

		friend class SceneHierarchyPanel;
	};
}
