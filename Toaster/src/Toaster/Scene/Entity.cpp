#include "tstpch.h"
#include "Entity.hpp"

namespace tst
{

	Entity::Entity(entt::entity handle, Scene *scene)
		: m_handle(handle), m_scene(scene)
	{
		
	}

}