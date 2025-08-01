#include "tstpch.h"
#include "RenderCommand.hpp"

#include "platform/OpenGl/OpenGLRendererAPI.hpp"

namespace tst
{
	RendererAPI* RenderCommand::m_rendererApi = new OpenGLRendererAPI;

	void RenderCommand::init()
	{
		m_rendererApi->init();
	}


	void RenderCommand::drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count)
	{
		m_rendererApi->drawIndexed(vertexArray, count);
	}

	void RenderCommand::drawIndexedBaseVertex(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexOffset, uint32_t baseVertex)
	{
		m_rendererApi->drawIndexedBaseVertex(vertexArray, indexCount, indexOffset, baseVertex);
	}


	void RenderCommand::drawArrays(const RefPtr<VertexArray>& vertexArray)
	{
		m_rendererApi->drawArrays(vertexArray);
	}

	void RenderCommand::clear()
	{
		m_rendererApi->clear();
	}

	void RenderCommand::disableDepthTesting()
	{
		m_rendererApi->disableDepthTesting();
	}

	void RenderCommand::enableDepthTesting()
	{
		m_rendererApi->enableDepthTesting();
	}

	void RenderCommand::setClearColour(const glm::vec4& colour)
	{
		m_rendererApi->setClearColour(colour);
	}

	void RenderCommand::resizeViewport(uint32_t width, uint32_t height)
	{
		m_rendererApi->resizeViewport(width, height);
	}


}
