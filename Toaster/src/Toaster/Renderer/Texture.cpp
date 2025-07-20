#include "tstpch.h"

#include "Texture.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLTexture.hpp"

namespace tst
{

	//-------------------------------Texture 2D-------------------------------

	RefPtr<Texture2D> Texture2D::create(const std::string& filepath)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(filepath); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const ColourRgba &colour)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(colour); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const ColourFloat& colour)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(colour); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const uint32_t colour)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(colour); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	//-------------------------------Texture 3D-------------------------------

	RefPtr<Texture3D> Texture3D::create(const std::vector<std::string>& texturePaths)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(texturePaths); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	/*RefPtr<Texture3D> Texture3D::create(const ColourRgba& colour)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(colour); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}*/
}
