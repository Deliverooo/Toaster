#pragma once
#include "Camera.hpp"
#include "Texture.hpp"
#include "glm/ext/matrix_transform.hpp"


namespace tst
{
	class TST_API Renderer2D
	{
	public:

		static glm::mat4 constructMatrix(const glm::vec3& p, const float r, const glm::vec2& s)
		{
			glm::mat4 sca = glm::scale(glm::mat4(1.0f), {s.x, s.y, 1.0f});
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(r), { 0.0f, 0.0f, 1.0f });
			glm::mat4 trans = glm::translate(glm::mat4(1.0f), p);
			return trans * rot * sca;
		}
		static glm::mat4 constructMatrix(const glm::vec2& p, const float r, const glm::vec2& s)
		{
			glm::mat4 sca = glm::scale(glm::mat4(1.0f), { s.x, s.y, 1.0f });
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(r), { 0.0f, 0.0f, 1.0f });
			glm::mat4 trans = glm::translate(glm::mat4(1.0f), {p.x, p.y, 0.0f});
			return trans * rot * sca;
		}

		static void init();
		static void terminate();

		static void begin(const RefPtr<OrthoCamera2D>& camera);
		static void begin(const ScopedPtr<OrthoCamera2D>& camera);
		static void end();

		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D> &texture, const float tilingScale = 1.0f, const glm::vec4 &tintColour = glm::vec4(1.0f));

		static void drawTri(const glm::vec3& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour);
		static void drawTri(const glm::vec2& position, const glm::vec2& scale, const float rotation, const glm::vec4& colour);


	};
}
