#include "tstpch.h"
#include "OpenGLFramebuffer.hpp"

namespace tst
{
	static bool isDepthFormat(FramebufferTextureFormat format)
	{
		return format == FramebufferTextureFormat::D32F_S8 || format == FramebufferTextureFormat::D24S8;
	}

	GLenum framebufferTextureFormatToOpenGL(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::RGBA8: return GL_RGBA;
		case FramebufferTextureFormat::RGBA4: return GL_RGBA;
		case FramebufferTextureFormat::RGB8: return GL_RGB;
		case FramebufferTextureFormat::RGBA16F: return GL_RGBA;
		case FramebufferTextureFormat::RGB16F: return GL_RGB16F;
		case FramebufferTextureFormat::RGBA32F: return GL_RGBA32F;
		case FramebufferTextureFormat::RGB32F: return GL_RGB32F;
		case FramebufferTextureFormat::D32F_S8: return GL_DEPTH24_STENCIL8; // OpenGL uses this for depth+stencil
		case FramebufferTextureFormat::D24S8: return GL_DEPTH24_STENCIL8; // OpenGL uses this for depth+stencil
		default:
			TST_ASSERT(false, "Unknown framebuffer texture format!");
			return 0;
		}
	}
	GLint framebufferTextureFormatToOpenGLInternal(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::RGBA8: return GL_RGBA8;
		case FramebufferTextureFormat::RGBA4: return GL_RGBA4;
		case FramebufferTextureFormat::RGB8: return GL_RGB8;
		case FramebufferTextureFormat::RGBA16F: return GL_RGBA16F;
		case FramebufferTextureFormat::RGB16F: return GL_RGB16F;
		case FramebufferTextureFormat::RGBA32F: return GL_RGBA32F;
		case FramebufferTextureFormat::RGB32F: return GL_RGB32F;
		case FramebufferTextureFormat::D32F_S8: return GL_DEPTH24_STENCIL8; // OpenGL uses this for depth+stencil
		case FramebufferTextureFormat::D24S8: return GL_DEPTH24_STENCIL8; // OpenGL uses this for depth+stencil
		default:
			TST_ASSERT(false, "Unknown framebuffer texture format!");
			return 0;
		}
	}
	GLint framebufferTextureWrappingToOpenGL(FramebufferTextureWrapping wrapping)
	{
		switch (wrapping)
		{
		case FramebufferTextureWrapping::Repeat: return GL_REPEAT;
		case FramebufferTextureWrapping::ClampToEdge: return GL_CLAMP_TO_EDGE;
		case FramebufferTextureWrapping::ClampToBorder: return GL_CLAMP_TO_BORDER;
		case FramebufferTextureWrapping::MirroredRepeat: return GL_MIRRORED_REPEAT;
		default:
			TST_ASSERT(false, "Unknown framebuffer texture wrapping!");
			return 0;
		}
	}
	GLint framebufferTextureFilteringToOpenGL(FramebufferTextureFiltering filtering)
	{
		switch (filtering)
		{
		case FramebufferTextureFiltering::Nearest: return GL_NEAREST;
		case FramebufferTextureFiltering::Linear: return GL_LINEAR;
		case FramebufferTextureFiltering::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
		case FramebufferTextureFiltering::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
		case FramebufferTextureFiltering::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
		case FramebufferTextureFiltering::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
		default:
			TST_ASSERT(false, "Unknown framebuffer texture filtering!");
			return 0;
		}
	}
	GLenum framebufferPixelDataTypeToOpenGL(FramebufferPixelDataType type)
	{
		switch (type)
		{
		case FramebufferPixelDataType::UnsignedByte: return GL_UNSIGNED_BYTE;
		case FramebufferPixelDataType::Byte: return GL_BYTE;
		case FramebufferPixelDataType::UnsignedShort: return GL_UNSIGNED_SHORT;
		case FramebufferPixelDataType::Short: return GL_SHORT;
		case FramebufferPixelDataType::UnsignedInt: return GL_UNSIGNED_INT;
		case FramebufferPixelDataType::Int: return GL_INT;
		case FramebufferPixelDataType::Float: return GL_FLOAT;
		default:
			TST_ASSERT(false, "Unknown framebuffer pixel data type!");
			return 0;
		}
	}

	GLenum framebufferTextureFormatToOpenGLType(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::None: return GL_NONE;
		case FramebufferTextureFormat::RGB8: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::RGBA8: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::RGBA4: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::RGBA16F: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::RGB16F: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::RGBA32F: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::RGB32F: return GL_COLOR_ATTACHMENT0;
		case FramebufferTextureFormat::D32F_S8: return GL_DEPTH_STENCIL_ATTACHMENT;
		case FramebufferTextureFormat::D24S8: return GL_DEPTH_STENCIL_ATTACHMENT;
		default:
			TST_ASSERT(false, "Unknown framebuffer attachment type!");
			return 0;
		}
	}

	static GLenum textureTarget(bool multisampled)
	{
		return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void createTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(textureTarget(multisampled), count, outID);
	}

	static void bindTexture(bool multisampled, uint32_t id)
	{
		glBindTexture(textureTarget(multisampled), id);
	}

	static void attachColourTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, textureTarget(multisampled), id, 0);
	}

	static void attachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, textureTarget(multisampled), id, 0);
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &m_framebufferId);
		glDeleteTextures(static_cast<GLint>(m_colourAttachments.size()), m_colourAttachments.data());
		glDeleteTextures(1, &m_depthAttachment);
	}
	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferCreateInfo& createInfo)
		: m_info(createInfo)
	{

		for (auto info : m_info.attachments.textures)
		{
			if (isDepthFormat(info.format))
			{
				m_depthAttachmentCreateInfo = info;
			} else
			{
				m_colourAttachmentCreateInfos.push_back(info);
			}
		}

		invalidate();
	}
	void OpenGLFramebuffer::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
		glViewport(0, 0, static_cast<GLint>(m_info.width), static_cast<GLint>(m_info.height));
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
			glDeleteTextures(static_cast<GLint>(m_colourAttachments.size()), m_colourAttachments.data());
			glDeleteTextures(1, &m_depthAttachment);

			m_colourAttachments.clear();
			m_depthAttachment = 0;
		}

		glGenFramebuffers(1, &m_framebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

		bool multisample = m_info.samples > 1;

		if (!m_colourAttachmentCreateInfos.empty())
		{
			m_colourAttachments.resize(m_colourAttachmentCreateInfos.size());
			createTextures(multisample, m_colourAttachments.data(), static_cast<uint32_t>(m_colourAttachments.size()));

			for (size_t i = 0; i < m_colourAttachments.size(); i++)
			{
				bindTexture(multisample, m_colourAttachments[i]);
				attachColourTexture(m_colourAttachments[i], static_cast<GLint>(m_info.samples), framebufferTextureFormatToOpenGLInternal(m_colourAttachmentCreateInfos[i].format), framebufferTextureFormatToOpenGL(m_colourAttachmentCreateInfos[i].format), m_info.width, m_info.height, static_cast<GLint>(i));

			}
		}

		if (m_depthAttachmentCreateInfo.format != FramebufferTextureFormat::None)
		{
			createTextures(multisample, &m_depthAttachment, 1);
			bindTexture(multisample, m_depthAttachment);

			attachDepthTexture(m_depthAttachment, static_cast<GLint>(m_info.samples), framebufferTextureFormatToOpenGL(m_depthAttachmentCreateInfo.format), framebufferTextureFormatToOpenGLType(m_depthAttachmentCreateInfo.format), m_info.width, m_info.height);
		}

		if (m_colourAttachments.size() > 1)
		{
			TST_ASSERT(m_colourAttachments.size() <= 4, "OpenGL only supports up to 4 colour attachments!");
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(m_colourAttachments.size(), buffers);
		}
		else if (m_colourAttachments.empty())
		{
			glDrawBuffer(GL_NONE);
		}

		TST_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::resize(uint32_t width, uint32_t height)
	{
		TST_PROFILE_FN();

		if (width == 0 || height == 0 || width > 4096 || height > 4096)
		{
			TST_CORE_ERROR("Cannot resize framebuffer to zero dimensions!");
			m_info.width = 1;
			m_info.height = 1;

			invalidate();

			return;
		}

		m_info.width = width;
		m_info.height = height;

		invalidate();
	}

}
