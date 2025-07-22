#pragma once

#include "RendererAPI.hpp"

namespace tst
{
	class RenderCommand
	{
	public:

		static void init();
		
		static void clear();
		static void setClearColour(const glm::vec4& colour);

		static void drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count = 0);
		static void drawArrays(const RefPtr<VertexArray>& vertexArray);

		static void resizeViewport(uint32_t width, uint32_t height);

	private:
		static RendererAPI* m_rendererApi;
	};
}


