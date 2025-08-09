#include "tstpch.h"
#include "RenderCommand.hpp"

#include "platform/OpenGl/OpenGLRendererAPI.hpp"

namespace tst
{
	RendererAPI* GraphicsAPI::m_rendererApi = new OpenGLRendererAPI;

	void GraphicsAPI::init()
	{
		m_rendererApi->init();
	}


	void GraphicsAPI::drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count)
	{
		m_rendererApi->drawIndexed(vertexArray, count);
	}

	void GraphicsAPI::drawIndexedBaseVertex(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexOffset, uint32_t baseVertex)
	{
		GraphicsAPI::m_rendererApi->drawIndexedBaseVertex(vertexArray, indexCount, indexOffset, baseVertex);
	}


	void GraphicsAPI::drawArrays(const RefPtr<VertexArray>& vertexArray, const uint32_t count, const DrawMode drawMode)
	{
		m_rendererApi->drawArrays(vertexArray, count, drawMode);
	}

	void GraphicsAPI::clear()
	{
		m_rendererApi->clear();
	}

	void GraphicsAPI::cleanState()
	{
		m_rendererApi->cleanState();
	}

	void GraphicsAPI::disableDepthTesting()
	{
		m_rendererApi->disableDepthTesting();
	}

	void GraphicsAPI::enableDepthTesting()
	{
		m_rendererApi->enableDepthTesting();
	}

	void GraphicsAPI::enableDepthMask()
	{
		m_rendererApi->enableDepthMask();
	}

	void GraphicsAPI::disableDepthMask()
	{
		m_rendererApi->disableDepthMask();
	}

	void GraphicsAPI::setDepthFunc(DepthFunc func)
	{
		m_rendererApi->setDepthFunc(func);
	}

#ifdef _DEBUG
#ifdef TST_API_ENABLE_CHECK_ERRORS
	void GraphicsAPI::checkError(const std::string& operation)
	{
		m_rendererApi->checkError(operation);
	}
#endif
#endif


	void GraphicsAPI::enableBackfaceCulling()
	{
		m_rendererApi->enableBackfaceCulling();
	}

	void GraphicsAPI::disableBackfaceCulling()
	{
		m_rendererApi->disableBackfaceCulling();
	}

	void GraphicsAPI::setClearColour(const glm::vec4& colour)
	{
		m_rendererApi->setClearColour(colour);
	}

	void GraphicsAPI::resizeViewport(uint32_t width, uint32_t height)
	{
		m_rendererApi->resizeViewport(width, height);
	}


}
