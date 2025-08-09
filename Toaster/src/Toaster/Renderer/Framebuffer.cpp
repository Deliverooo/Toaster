#include "tstpch.h"
#include "Framebuffer.hpp"

#include "platform/OpenGl/OpenGLFramebuffer.hpp"
#include "Renderer.hpp"

namespace tst
{

	RefPtr<Framebuffer> Framebuffer::create(const FramebufferCreateInfo& createInfo)
	{
		switch (RendererAPI::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLFramebuffer>(createInfo); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}
}
