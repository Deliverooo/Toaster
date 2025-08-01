#pragma once
#include "Toaster/Renderer/RendererAPI.hpp"

namespace tst
{
	class TST_API OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void init() override;

		virtual void clear() override;
		virtual void setClearColour(const glm::vec4 &colour) override;

		virtual void disableDepthTesting() override;
		virtual void enableDepthTesting() override;

		virtual void drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count = 0) override;
		virtual void drawIndexedBaseVertex(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexOffset, uint32_t baseVertex) override;
		virtual void drawArrays(const RefPtr<VertexArray>& vertexArray) override;

		virtual void resizeViewport(uint32_t width, uint32_t height) override;

	private:
	};
}
