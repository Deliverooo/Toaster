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
			if (hasComponent<T>())
			{
				TST_CORE_WARN("Entity already has component");
				return getComponent<T>();
			}

			T &component = m_scene->registry().emplace<T>(m_handle, std::forward<Args>(args)...);
			m_scene->onComponentAdd<T>(this, component);
			return component;
		}

		template<typename T, typename ...Args>
		T& addComponent(const Args &&...args)
		{
			if (hasComponent<T>())
			{
				TST_CORE_WARN("Entity already has component");
				return getComponent<T>();
			}
			T& component = m_scene->registry().emplace<T>(m_handle, std::forward<const Args>(args)...);
			m_scene->onComponentAdd<T>(this, component);
			return component;
		}

		template<typename T>
		T& addComponent()
		{
			if (hasComponent<T>())
			{
				TST_CORE_WARN("Entity already has component");
				return getComponent<T>();
			}

			T& component = m_scene->registry().emplace<T>(m_handle);
			m_scene->onComponentAdd<T>(this, component);
			return component;
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


		operator entt::entity() const { return m_handle; }

	private:

		entt::entity m_handle{ entt::null };
		Scene* m_scene{ nullptr };
	};

	
}
