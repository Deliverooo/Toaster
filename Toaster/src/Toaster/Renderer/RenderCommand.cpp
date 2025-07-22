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

	void RenderCommand::drawArrays(const RefPtr<VertexArray>& vertexArray)
	{
		m_rendererApi->drawArrays(vertexArray);
	}

	void RenderCommand::clear()
	{
		m_rendererApi->clear();
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
