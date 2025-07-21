#pragma once
#include "Camera.hpp"
#include "RendererAPI.hpp"
#include "Shader.hpp"

namespace tst
{
	class TST_API Renderer
	{
	public:

		static void init();
		static void terminate();

		static RendererAPI::API getApi() { return RendererAPI::getApi(); }

		static void begin(const RefPtr<Camera>& camera);
		static void begin(const RefPtr<PerspectiveCamera>& camera);
		static void begin(const RefPtr<OrthoCamera>& camera);
		static void begin(const RefPtr<OrthoCamera2D>& camera);

		static void end();

		static void submit(const RefPtr<VertexArray>& vertexArray, const RefPtr<Shader>& shader, const glm::mat4& transform);

		static void resizeViewport(uint32_t width, uint32_t height);

	private:
		struct SceneData
		{
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrix;
		};

		static SceneData* m_sceneData;
	};
}
