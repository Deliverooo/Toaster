#pragma once
#include "VertexArray.hpp"
#include "glm/glm.hpp"

namespace tst
{
	class TST_API RendererAPI
	{
	public:
		enum class API
		{
			None, OpenGL, Vulkan, DirectX,
		};

		virtual void init() = 0;

		virtual void clear() = 0;
		virtual void setClearColour(const glm::vec4 &colour) = 0;

		virtual void disableDepthTesting() = 0;
		virtual void enableDepthTesting() = 0;

		virtual void drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count) = 0;
		virtual void drawIndexedBaseVertex(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexOffset, uint32_t baseVertex) = 0;
		virtual void drawArrays(const RefPtr<VertexArray>& vertexArray) = 0;

		virtual void resizeViewport(uint32_t width, uint32_t height) = 0;

		static API getApi() { return m_Api; }

	private:
		static API m_Api;
	};
}
