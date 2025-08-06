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
			glm::mat4 sca = glm::scale(glm::mat4(1.0f), { s.x, s.y, 1.0f });
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(r), { 0.0f, 0.0f, 1.0f });
			glm::mat4 trans = glm::translate(glm::mat4(1.0f), p);
			return trans * rot * sca;
		}
		static glm::mat4 constructMatrix(const glm::vec2& p, const float r, const glm::vec2& s)
		{
			glm::mat4 sca = glm::scale(glm::mat4(1.0f), { s.x, s.y, 1.0f });
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(r), { 0.0f, 0.0f, 1.0f });
			glm::mat4 trans = glm::translate(glm::mat4(1.0f), { p.x, p.y, 0.0f });
			return trans * rot * sca;
		}

		static void init();
		static void terminate();

		static uint32_t enumerateTextureIndex(const RefPtr<Texture2D>& texture);

		static void begin(const RefPtr<OrthoCamera2D>& camera);
		static void begin(const Camera& camera, const glm::mat4& transform);
		static void begin(const EditorCamera& camera);
		static void begin(const RefPtr<PerspectiveCamera>& camera);

		static void end();

		static void flush();
		static void beginNewBatch();

		static void drawQuad(const glm::mat4 &transform, const glm::vec4& colour);
		static void drawQuad(const glm::mat4 &transform, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::mat4 &transform, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		static void drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		struct Stats
		{
			uint32_t drawCallCount{ 0 };
			uint32_t quadCount{ 0 };
			uint32_t triangleCount{ 0 };
			uint32_t circleCount{ 0 };
			uint32_t lineCount{ 0 };
			uint32_t batchesPerFrame{ 1 };
			uint32_t textureBindings{ 0 };
			uint32_t verticesSubmitted{ 0 };

			uint32_t totalVertexCount() { return quadCount * 4 + triangleCount * 3 + circleCount * 4 + lineCount * 4; }
			uint32_t totalIndexCount() { return quadCount * 6 + triangleCount * 6 + circleCount * 6 + lineCount * 6; }
			float batchEfficiency() { return drawCallCount > 0 ? static_cast<float>(quadCount) / static_cast<float>(drawCallCount) : 0.0f; }
		};

		static Stats getStats();
		static void resetStats();

	};
}