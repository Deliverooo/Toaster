#include "tstpch.h"

#include "Buffer.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLBuffer.hpp"

namespace tst
{
	std::shared_ptr<VertexBuffer> VertexBuffer::create(float* vertices, uint32_t size)
	{
		switch (Renderer::getApi())
		{
		case RenderApi::None:
			{
			TST_ASSERT(false, "Render Api cannot be None!");
			return nullptr;
			}
		case RenderApi::OpenGL:
			{
			return std::make_shared<OpenGLVertexBuffer>(vertices, size);
			}
		case RenderApi::Vulkan:
			{
			TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented...");
			return nullptr;
			}
		default:
			{
			TST_ASSERT(false, "Unknown Render API specified");
			return nullptr;
			}
		}
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::getApi())
		{
		case RenderApi::None:
			{
			TST_ASSERT(false, "Render Api cannot be None!");
			return nullptr;
			}
		case RenderApi::OpenGL:
			{
			return std::make_shared<OpenGLIndexBuffer>(indices, count);
			}
		case RenderApi::Vulkan:
			{
			TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented...");
			return nullptr;
			}
		default:
			{
			TST_ASSERT(false, "Unknown Render API specified");
			return nullptr;
			}
		}
	}	
}
