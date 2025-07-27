#include "tstpch.h"
#include "Scene.hpp"

#include <glm/glm.hpp>

namespace tst
{

	struct Transform
	{
		Transform(const glm::mat4& transform) : matrix(transform) {}

		operator glm::mat4& (){return matrix; }
		operator const glm::mat4& () const {return matrix; }

		union
		{
			glm::mat4 transform;
			glm::mat4 matrix;
		};
	};

	Scene::Scene()
	{

		entt::entity entity = m_registry.create();
		
		m_registry.emplace<Transform>(entity, glm::mat4(1.0f));
	}

	Scene::~Scene()
	{
		
	}


}
