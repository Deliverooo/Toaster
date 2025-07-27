#pragma once
#include "Texture.hpp"

namespace tst
{
	struct FramebufferCreateInfo
	{
		uint32_t width;
		uint32_t height;
	};
	class TST_API Framebuffer
	{
	public:
		virtual ~Framebuffer(){};

		static RefPtr<Framebuffer> create(const FramebufferCreateInfo& createInfo);

		virtual const FramebufferCreateInfo& getInfo() const = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual const uint32_t &getColourAttachmentId() const = 0;
		virtual const uint32_t &getDepthAttachmentId() const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
	};
}
