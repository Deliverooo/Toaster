#include "tstpch.h"
#include "OpenGLFramebuffer.hpp"

namespace tst
{
	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &m_framebufferId);
		glDeleteTextures(1, &m_colourAttachmentId);
		glDeleteTextures(1, &m_depthAttachmentId);
	}
	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferCreateInfo& createInfo)
		: m_info(createInfo)
	{
		glGenFramebuffers(1, &m_framebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

		glGenTextures(1, &m_colourAttachmentId);
		glBindTexture(GL_TEXTURE_2D, m_colourAttachmentId);

		// Allocate texture storage
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_info.width, m_info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colourAttachmentId, 0);

		glGenTextures(1, &m_depthAttachmentId);
		glBindTexture(GL_TEXTURE_2D, m_depthAttachmentId);

		// Allocate texture storage
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_info.width, m_info.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthAttachmentId, 0);

		TST_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void OpenGLFramebuffer::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
		glViewport(0, 0, m_info.width, m_info.height);
	}
	void OpenGLFramebuffer::unbind() const
	{
		TST_PROFILE_FN();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::invalidate()
	{
		if (m_framebufferId)
		{
			glDeleteFramebuffers(1, &m_framebufferId);
			glDeleteTextures(1, &m_colourAttachmentId);
			glDeleteTextures(1, &m_depthAttachmentId);
		}

		glGenFramebuffers(1, &m_framebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

		glGenTextures(1, &m_colourAttachmentId);
		glBindTexture(GL_TEXTURE_2D, m_colourAttachmentId);

		// Allocate texture storage
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_info.width, m_info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colourAttachmentId, 0);

		glGenTextures(1, &m_depthAttachmentId);
		glBindTexture(GL_TEXTURE_2D, m_depthAttachmentId);

		// Allocate texture storage
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_info.width, m_info.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthAttachmentId, 0);

		TST_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::resize(uint32_t width, uint32_t height)
	{
		TST_PROFILE_FN();

		if (width == 0 || height == 0 || width > 4096 || height > 4096)
		{
			TST_ASSERT(false, "Cannot resize framebuffer to zero dimensions!");
			return;
		}

		m_info.width = width;
		m_info.height = height;

		invalidate();
	}
}
