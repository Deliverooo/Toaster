#pragma once

#include "Entity.hpp"

namespace tst
{
	class ScriptableEntity
	{
	public:

		virtual ~ScriptableEntity() = default;

		template<typename T>
		[[nodiscard]] T& getComponent()
		{
			return m_entity.getComponent<T>();
		}

	protected:

		virtual void onCreate() {}
		virtual void onDestroy() {}
		virtual void onUpdate(DeltaTime dt) {}

		Entity m_entity;
		friend class Scene;
	};
}
