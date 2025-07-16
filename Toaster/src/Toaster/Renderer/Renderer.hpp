#pragma once
#include "Camera.hpp"
#include "RendererAPI.hpp"
#include "Shader.hpp"

namespace tst
{
	class TST_API Renderer
	{
	public:
		static RendererAPI::API getApi() { return RendererAPI::getApi(); }


		static void begin(const std::shared_ptr<Camera>& camera);
		//static void begin(const std::shared_ptr<Camera>& camera);
		static void end();

		static void submit(const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform, const std::shared_ptr<Shader>& shader);

	private:
		struct SceneData
		{
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrix;
		};

		static SceneData* m_sceneData;
	};
}
