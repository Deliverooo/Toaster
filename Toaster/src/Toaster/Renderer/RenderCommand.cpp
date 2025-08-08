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


	void RenderCommand::drawArrays(const RefPtr<VertexArray>& vertexArray, const uint32_t count, const DrawMode drawMode)
	{
		m_rendererApi->drawArrays(vertexArray, count, drawMode);
	}

	void RenderCommand::clear()
	{
		m_rendererApi->clear();
	}

	void RenderCommand::cleanState()
	{
		m_rendererApi->cleanState();
	}


	void RenderCommand::disableDepthTesting()
	{
		m_rendererApi->disableDepthTesting();
	}

	void RenderCommand::enableDepthTesting()
	{
		m_rendererApi->enableDepthTesting();
	}

	void RenderCommand::enableDepthMask()
	{
		m_rendererApi->enableDepthMask();
	}

	void RenderCommand::disableDepthMask()
	{
		m_rendererApi->disableDepthMask();
	}

	void RenderCommand::setDepthFunc(DepthFunc func)
	{
		m_rendererApi->setDepthFunc(func);
	}

#ifdef _DEBUG
#ifdef TST_API_ENABLE_CHECK_ERRORS
	void RenderCommand::checkError(const std::string& operation)
	{
		m_rendererApi->checkError(operation);
	}
#endif
#endif


	void RenderCommand::enableBackfaceCulling()
	{
		m_rendererApi->enableBackfaceCulling();
	}

	void RenderCommand::disableBackfaceCulling()
	{
		m_rendererApi->disableBackfaceCulling();
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
