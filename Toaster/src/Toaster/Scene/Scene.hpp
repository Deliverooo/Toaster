#pragma once

#include "entt.hpp"
#include "Toaster/Core/Time.hpp"

namespace tst
{

	class Entity;

	class Scene
	{
	public:

		Scene();
		~Scene();

		void onUpdate(DeltaTime dt);

		Entity createEntity();

		entt::registry &registry() { return m_registry; }

	private:

		entt::registry m_registry;
	};
}
