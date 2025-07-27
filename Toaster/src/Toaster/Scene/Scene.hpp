#pragma once

#include "entt.hpp"

namespace tst
{
	class Scene
	{
	public:

		Scene();
		~Scene();

	private:

		entt::registry m_registry;
	};
}
