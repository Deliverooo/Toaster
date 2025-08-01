#pragma once

#include "RendererAPI.hpp"

namespace tst
{
	class RenderCommand
	{
	public:

		static void init();
		
		static void clear();
		static void disableDepthTesting();
		static void enableDepthTesting();
		static void setClearColour(const glm::vec4& colour);

		static void drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count = 0);
		static void drawIndexedBaseVertex(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexOffset, uint32_t baseVertex);

		static void drawArrays(const RefPtr<VertexArray>& vertexArray);

		static void resizeViewport(uint32_t width, uint32_t height);

	private:
		static RendererAPI* m_rendererApi;
	};
}


