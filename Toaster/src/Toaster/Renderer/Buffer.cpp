#include "tstpch.h"

#include "Buffer.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLBuffer.hpp"

namespace tst
{
	RefPtr<VertexBuffer> VertexBuffer::create(float* vertices, uint32_t size)
	{
		switch (RendererAPI::getApi())
		{
		case RendererAPI::API::None:
			{
			TST_ASSERT(false, "Render Api cannot be None!");
			return nullptr;
			}
		case RendererAPI::API::OpenGL:
			{
			return std::make_shared<OpenGLVertexBuffer>(vertices, size);
			}
		case RendererAPI::API::Vulkan:
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
	RefPtr<VertexBuffer> VertexBuffer::create(uint32_t size)
	{
		switch (RendererAPI::getApi())
		{
		case RendererAPI::API::None:
			{
			TST_ASSERT(false, "Render Api cannot be None!");
			return nullptr;
			}
		case RendererAPI::API::OpenGL:
			{
			return std::make_shared<OpenGLVertexBuffer>(size);
			}
		case RendererAPI::API::Vulkan:
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

	RefPtr<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		switch (RendererAPI::getApi())
		{
		case RendererAPI::API::None:
			{
			TST_ASSERT(false, "Render Api cannot be None!");
			return nullptr;
			}
		case RendererAPI::API::OpenGL:
			{
			return std::make_shared<OpenGLIndexBuffer>(indices, count);
			}
		case RendererAPI::API::Vulkan:
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
