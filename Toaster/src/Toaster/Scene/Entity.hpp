#pragma once

#include "Scene.hpp"
#include "Components.hpp"
#include "entt.hpp"


namespace tst
{

	class Entity
	{
	public:

		Entity() = default;
		Entity(entt::entity handle, Scene *scene);

		template<typename T>
		[[nodiscard]] T& getComponent()
		{
			TST_ASSERT(hasComponent<T>(), "Entity does not have component!");
			return m_scene->registry().get<T>(m_handle);
		}

		template<typename ...Types>
		bool hasComponent()	{ return m_scene->registry().all_of<Types...>(m_handle); }


		template<typename T, typename ...Args>
		T& addComponent(Args &...args)
		{
			TST_ASSERT(!hasComponent<T>(), "Entity already has component!");
			return m_scene->registry().emplace<T>(m_handle, std::forward<Args>(args)...);
		}

		template<typename T, typename ...Args>
		T& addComponent(const Args &&...args)
		{
			TST_ASSERT(!hasComponent<T>(), "Entity already has component!");
			return m_scene->registry().emplace<T>(m_handle, std::forward<const Args>(args)...);
		}

		template<typename T>
		T& addComponent()
		{
			TST_ASSERT(!hasComponent<T>(), "Entity already has component!");
			return m_scene->registry().emplace<T>(m_handle);
		}


		template<typename T>
		void removeComponent()
		{
			TST_ASSERT(hasComponent<T>(), "Entity does not have component!");
			m_scene->registry().erase<T>(m_handle);
		}

		operator uint32_t() const { return static_cast<uint32_t>(m_handle); }

		operator bool() const { return m_handle != entt::null; }

		bool operator==(const Entity& other) const { return m_handle == other.m_handle && m_scene == other.m_scene; }
		bool operator!=(const Entity& other) const { return m_handle != other.m_handle || m_scene != other.m_scene; }

	private:

		entt::entity m_handle{ entt::null };
		Scene* m_scene{ nullptr };
	};

	
}
