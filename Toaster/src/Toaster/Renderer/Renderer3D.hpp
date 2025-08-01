#pragma once


#include "Camera.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Texture.hpp"
#include "Toaster/Scene/Components.hpp"


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
		static void terminate() noexcept;

		static void begin(const RefPtr<PerspectiveCamera>& camera);
		static void begin(const RefPtr<OrthoCamera>& camera);
		static void begin(const Camera& camera, const glm::mat4 &transform);

		static void end();

		static void flush();
		static void beginNewBatch();


		// Quad rendering
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::mat4& transform, const glm::vec4& colour);
		static void drawQuad(const glm::mat4& transform, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::mat4& transform, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Cube rendering
		static void drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour);
		static void drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawCube(const glm::mat4& transform, const glm::vec4& colour);
		static void drawCube(const glm::mat4& transform, const RefPtr<Texture2D>& texture, float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawCube(const glm::mat4& transform, const RefPtr<SubTexture2D>& texture, float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Mesh rendering - Add these declarations
		static void drawMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& colour = glm::vec4(1.0f));
		static void drawMesh(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Mesh>& mesh, const glm::vec4& colour = glm::vec4(1.0f));

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
			uint32_t meshCount{ 0 };

			uint32_t totalVertexCount() { return quadCount * 4 + triangleCount * 3 + circleCount * 4 + lineCount * 4; }
			uint32_t totalIndexCount() { return quadCount * 6 + triangleCount * 6 + circleCount * 6 + lineCount * 6; }
			float batchEfficiency() { return drawCallCount > 0 ? static_cast<float>(quadCount) / static_cast<float>(drawCallCount) : 0.0f; }
		};

		static Stats getStats();
		static void resetStats();

	private:
		static uint32_t enumerateTextureIndex(const RefPtr<Texture2D>& texture);
		
	};
	
}
