#include "tstpch.h"
#include "OpenGLFramebuffer.hpp"

namespace tst
{
	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		TST_PROFILE_FN();
		glDeleteFramebuffers(1, &m_framebufferId);
	}
	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferCreateInfo& createInfo)
		: m_info(createInfo)
	{
		TST_PROFILE_FN();
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
		TST_PROFILE_FN();
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
	}
	void OpenGLFramebuffer::unbind() const
	{
		TST_PROFILE_FN();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void OpenGLFramebuffer::resize(uint32_t width, uint32_t height)
	{
		TST_PROFILE_FN();

		m_info.width = width;
		m_info.height = height;

		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
		
		glViewport(0, 0, width, height);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}
