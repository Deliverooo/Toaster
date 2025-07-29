#pragma once
#include "glm/ext/matrix_transform.hpp"
#include "Camera.hpp"
#include "RendererAPI.hpp"
#include "Shader.hpp"
#include "Texture.hpp"

#define TST_CORE_ASSET_PATH "C:/dev/Toaster/Toaster/res/"

namespace tst
{

	class TST_API Renderer
	{
	public:



		static RendererAPI::API getApi() { return RendererAPI::getApi(); }


		//static void submit(const RefPtr<VertexArray>& vertexArray, const RefPtr<Shader>& shader, const glm::mat4& transform);

		static void resizeViewport(uint32_t width, uint32_t height);


		static void init();
		static void terminate();

		static void begin(const RefPtr<PerspectiveCamera>& camera);
		static void begin(const RefPtr<OrthoCamera>& camera);
		static void begin(const RefPtr<OrthoCamera2D>& camera);

		static void end();

		static void flush();
		static void beginNewBatch();


		// ---------2D Primitive drawing functions---------

		// Quads with 2D position
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Quads with 2D position + layer depth
		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Tri with 2D position
		static void drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawTri(const glm::vec2& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Tri with 2D position + layer depth
		static void drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawTri(const glm::vec3& position, const float rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// ---[ 3D rotation overloads ]---

		// Quads with 2 scale parameters
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Quads with 3 scale parameters
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour);
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Tris with 2 scale parameters
		static void drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const glm::vec4& colour);
		static void drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec2& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));

		// Tris with 3 scale parameters
		static void drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour);
		static void drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawTri(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));



		// ---------3D Primitive drawing functions---------


		// Cube
		static void drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const glm::vec4& colour);
		static void drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<Texture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));
		static void drawCube(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale, const RefPtr<SubTexture2D>& texture, const float tilingScale = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f));


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

	private:
		struct SceneData
		{
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrix;
		};

		static SceneData* m_sceneData;
	};
}
