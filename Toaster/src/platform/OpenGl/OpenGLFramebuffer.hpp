#pragma once
#include "Toaster/Renderer/Framebuffer.hpp"
#include <glad/glad.h>


namespace tst
{
	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		virtual ~OpenGLFramebuffer() override;

		OpenGLFramebuffer(const FramebufferCreateInfo& createInfo);

		virtual const FramebufferCreateInfo& getInfo() const override { return m_info; }

		virtual void bind() const override;
		virtual void unbind() const override;
		virtual void resize(uint32_t width, uint32_t height) override;

		virtual const uint32_t &getColourAttachmentId() const override { return m_colourAttachmentId; }
		virtual const uint32_t &getDepthAttachmentId() const override { return m_depthAttachmentId; }

	private:

		void invalidate();

		FramebufferCreateInfo m_info;

		uint32_t m_framebufferId{ 0 };

		uint32_t m_colourAttachmentId{ 0 };
		uint32_t m_depthAttachmentId{ 0 };
	};
}
