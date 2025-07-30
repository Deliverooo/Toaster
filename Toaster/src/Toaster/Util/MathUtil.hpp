#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

static inline constexpr glm::mat4 transformationMat(const glm::vec3& positon, const glm::vec3& rotation, const glm::vec3& scale)
{
	return glm::translate(glm::mat4(1.0f), positon)
		* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::mod(glm::length(rotation), glm::two_pi<float>()), glm::normalize(rotation)))
		* glm::scale(glm::mat4(1.0f), scale);
}

static inline constexpr glm::mat4 transformationMat(const glm::vec2& positon, const float rotation, const glm::vec2& scale)
{
	return glm::translate(glm::mat4(1.0f), {positon.x, positon.y, 0.0f})
		* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::mod(rotation, glm::two_pi<float>()), {0.0f, 0.0f, 1.0f}))
		* glm::scale(glm::mat4(1.0f), {scale.x, scale.y, 1.0f});
}

static inline constexpr glm::mat4 transformationMat(const glm::vec3& positon, const float rotation, const glm::vec2& scale)
{
	return glm::translate(glm::mat4(1.0f), positon)
		* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::mod(rotation, glm::two_pi<float>()), { 0.0f, 0.0f, 1.0f }))
		* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 1.0f });
}