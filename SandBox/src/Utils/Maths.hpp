#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline glm::mat4 transformationMat(const glm::vec3 &positon, const glm::vec3 &rotation, const glm::vec3 &scale)
{
	return glm::translate(glm::mat4(1.0f), positon)
		* ((glm::length(rotation) == 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation)))
		* glm::scale(glm::mat4(1.0f), scale);
}
