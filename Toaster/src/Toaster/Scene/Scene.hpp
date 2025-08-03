 #pragma once

#include "Toaster/Core/Time.hpp"
#include "entt.hpp"


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
		void removeEntity(const Entity& entity);

		entt::registry &registry() { return m_registry; }

	private:

		template<typename T>
		void onComponentAdd(Entity *entity, T& component);

		entt::registry m_registry;

		uint32_t m_ViewportWidth;
		uint32_t m_ViewportHeight;

		friend class Entity;
		friend class SceneHierarchyPanel;
	};
}
