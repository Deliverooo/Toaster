#include "tstpch.h"
#include "VertexArray.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLVertexArray.hpp"

namespace tst
{
	std::shared_ptr<VertexArray> VertexArray::create()
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
			return std::make_shared<OpenGLVertexArray>();
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
