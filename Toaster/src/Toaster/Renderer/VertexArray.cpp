#include "tstpch.h"
#include "VertexArray.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLVertexArray.hpp"

namespace tst
{
	RefPtr<VertexArray> VertexArray::create()
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLVertexArray>(); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

}
