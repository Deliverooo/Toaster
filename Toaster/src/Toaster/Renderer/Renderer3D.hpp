#pragma once
#include "Camera.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Texture.hpp"

namespace tst
{
	class TST_API Renderer3D
	{
	public:

		static glm::mat4 constructMatrix(const glm::vec3& p, const glm::vec3& r, const glm::vec3& s)
		{
			glm::mat4 sca = glm::scale(glm::mat4(1.0f), s);
			glm::mat4 rot = (glm::length(r) <= 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(r), glm::normalize(r));
			glm::mat4 trans = glm::translate(glm::mat4(1.0f), p);
			return trans * rot * sca;
		}

		static glm::mat4 constructMatrix(const glm::vec3& p, const float r, const glm::vec3& s)
		{
			glm::mat4 sca = glm::scale(glm::mat4(1.0f), s);
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(r), {0.0f, 0.0f, 1.0f});
			glm::mat4 trans = glm::translate(glm::mat4(1.0f), p);
			return trans * rot * sca;
		}

		static void init();
		static void terminate();

		static void begin(const RefPtr<PerspectiveCamera>& camera);
		static void begin(const RefPtr<OrthoCamera>& camera);

		static void end();

		static void drawQuad(const glm::vec3& position, const glm::vec3 &rotation, const glm::vec3& scale, const glm::vec4& colour);

		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture);

	};
}
