#include "tstpch.h"

#include "Texture.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLTexture.hpp"

namespace tst
{
	std::shared_ptr<Texture> Texture::create(const std::string& filepath)
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
			return std::make_shared<OpenGLTexture>(filepath);
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
