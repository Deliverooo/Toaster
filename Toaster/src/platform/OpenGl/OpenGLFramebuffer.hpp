#pragma once
#include "Toaster/Renderer/Framebuffer.hpp"
#include <glad/glad.h>


namespace tst
{
	GLenum framebufferTextureFormatToOpenGL(FramebufferTextureFormat format);
	GLint framebufferTextureFormatToOpenGLInternal(FramebufferTextureFormat format);
	GLint framebufferTextureWrappingToOpenGL(FramebufferTextureWrapping wrapping);
	GLint framebufferTextureFilteringToOpenGL(FramebufferTextureFiltering filtering);
	GLenum framebufferPixelDataTypeToOpenGL(FramebufferPixelDataType type);


	GLenum framebufferTextureFormatToOpenGLType(FramebufferTextureFormat format);

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		virtual ~OpenGLFramebuffer() override;

		OpenGLFramebuffer(const FramebufferCreateInfo& createInfo);

		virtual const FramebufferCreateInfo& getInfo() const override { return m_info; }

		virtual void bind() const override;
		virtual void unbind() const override;
		virtual void resize(uint32_t width, uint32_t height) override;

		virtual int readPixel(uint32_t attachmentIndex, int x, int y) override;
		virtual void clearAttachment(uint32_t attachmentIndex, int value) override;

		virtual const uint32_t &getColourAttachmentId(uint32_t index) const override { return m_colourAttachments[index]; }
		virtual const uint32_t &getDepthAttachmentId() const override { return m_depthAttachment; }


	private:

		void invalidate();

		FramebufferCreateInfo m_info;

		uint32_t m_framebufferId{ 0 };

		std::vector<FramebufferTextureCreateInfo> m_colourAttachmentCreateInfos;
		FramebufferTextureCreateInfo m_depthAttachmentCreateInfo{ FramebufferTextureFormat::None };

		std::vector<uint32_t> m_colourAttachments;
		uint32_t m_depthAttachment{ 0 };
	};
}
