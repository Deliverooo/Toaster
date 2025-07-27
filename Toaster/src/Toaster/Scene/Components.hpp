#pragma once

#include <glm/glm.hpp>

#include "Toaster/Renderer/Texture.hpp"

namespace tst
{
	struct TransformComponent
	{
		TransformComponent() = default;
		TransformComponent(const glm::mat4& transform) : matrix(transform) {}

		operator glm::mat4& () { return matrix;	}
		operator const glm::mat4& () const {return matrix;}

		union
		{
			glm::mat4 matrix{ 1.0f };
			glm::mat4 transform;
		};
	};

	struct SpriteRendererComponent
	{
		SpriteRendererComponent() = default;
		SpriteRendererComponent(const glm::vec4& colour) : colour(colour) {}

		glm::vec4 colour{ 1.0f, 0.0f, 0.862f, 1.0f };
	};

	struct MaterialComponent
	{
		MaterialComponent() = default;
		MaterialComponent(const glm::vec4& colour) : colour(colour) {}

		
		glm::vec4 colour{ 1.0f, 0.0f, 0.862f, 1.0f };
	};



}
