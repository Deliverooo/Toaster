#pragma once

#include "Scene.hpp"

namespace tst
{
	class Entity
	{
	public:

		Entity(entt::entity handle, Scene *scene);

		template<typename T>
		T& getComponent() { return m_scene->registry().get<T>(m_handle);	}

		template<typename ...Types>
		bool hasComponent()	{ return m_scene->registry().all_of<Types...>(m_handle);}

		template<typename T, typename ...Args>
		void addComponent(Args... args) { m_scene->registry().emplace<T>(m_handle, args...); }

		template<typename T>
		void removeComponent() { m_scene->registry().erase<T>(m_handle); }


	private:

		entt::entity m_handle{ entt::null };
		Scene *m_scene;
	};
}