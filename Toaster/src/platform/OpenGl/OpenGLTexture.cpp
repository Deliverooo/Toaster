#include "tstpch.h"
#include "OpenGLTexture.hpp"

#include <stb_image.h>
#include <glad/glad.h>

namespace tst
{
	OpenGLTexture2D::OpenGLTexture2D(const std::string& filepath) : m_texturePath(filepath)
	{

		int width, height, nrChannels;
		unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);

		if (!data)
		{
			TST_CORE_ERROR("Failed to load image -> {0}", filepath);

			glGenTextures(1, &m_textureId);
			glBindTexture(GL_TEXTURE_2D, m_textureId);

			m_textureWidth = 1;
			m_textureHeight = 1;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			unsigned char invalidTextureData[3];
			invalidTextureData[0] = 255;
			invalidTextureData[1] = 0;
			invalidTextureData[2] = 220;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &invalidTextureData);

			return;
		}

		m_textureWidth	= width;
		m_textureHeight = height;

		GLenum format = 0;
		GLenum internalFormat = 0;

		switch (nrChannels)
		{
		case 3: format = GL_RGB; internalFormat = GL_RGB8; break;
		case 4: format = GL_RGBA; internalFormat = GL_RGBA8; break;
		}

		TST_ASSERT(format, "Invalid Texture Colour format");

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_textureWidth, m_textureHeight, 0, format, GL_UNSIGNED_BYTE, &data[0]);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data);
	}

	OpenGLTexture2D::OpenGLTexture2D(const ColourRgba& colour)
	{
		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		m_textureWidth	= 1;
		m_textureHeight = 1;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned char data[4];
		data[0] = static_cast<unsigned char>(colour.r);
		data[1] = static_cast<unsigned char>(colour.g);
		data[2] = static_cast<unsigned char>(colour.b);
		data[3] = static_cast<unsigned char>(colour.a);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureWidth, m_textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	}

	OpenGLTexture2D::OpenGLTexture2D(const ColourFloat& colour)
	{
		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		m_textureWidth	= 1;
		m_textureHeight = 1;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned char data[4];
		data[0] = static_cast<unsigned char>(colour.r * 255.0f);
		data[1] = static_cast<unsigned char>(colour.g * 255.0f);
		data[2] = static_cast<unsigned char>(colour.b * 255.0f);
		data[3] = static_cast<unsigned char>(colour.a * 255.0f);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureWidth, m_textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	}

	OpenGLTexture2D::OpenGLTexture2D(const uint32_t colour)
	{
		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		m_textureWidth	= 1;
		m_textureHeight = 1;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned char data[4];
		data[0] = static_cast<unsigned char>((colour & (0xff000000)) >> 24);
		data[1] = static_cast<unsigned char>((colour & (0x00ff0000)) >> 12);
		data[2] = static_cast<unsigned char>((colour & (0x0000ff00)) >> 6);
		data[3] = static_cast<unsigned char>((colour & (0x000000ff)) >> 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureWidth, m_textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	}

	OpenGLTexture2D::OpenGLTexture2D(const uint32_t width, const uint32_t height)
	{
		m_textureWidth = width;
		m_textureHeight = height;

		GLenum format = GL_RGBA;
		GLenum internalFormat = GL_RGBA8;

		TST_ASSERT(format, "Invalid Texture Colour format");

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	}


	void OpenGLTexture2D::setData(void* data, size_t size)
	{

		glBindTexture(GL_TEXTURE_2D, m_textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_textureWidth, m_textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	}


	OpenGLTexture2D::~OpenGLTexture2D()
	{
        glDeleteTextures(1, &m_textureId);
	}

	void OpenGLTexture2D::bind(uint32_t slot) const
	{
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
	}

	void OpenGLTexture2D::unbind() const
	{
        glActiveTexture(0);
        glBindTexture(GL_TEXTURE_2D, 0);
	}


	OpenGLTexture3D::OpenGLTexture3D(const std::vector<std::string>& texturePaths)
	{
		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		int width, height, nrChannels;
		for (unsigned int i = 0; i < texturePaths.size(); i++)
		{
			unsigned char* data = stbi_load(texturePaths[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
				);
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Cubemap tex failed to load at path: " << texturePaths[i] << std::endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	OpenGLTexture3D::OpenGLTexture3D(const ColourRgba& colour)
	{
		
	}

	OpenGLTexture3D::~OpenGLTexture3D()
	{
	}

	void OpenGLTexture3D::bind(uint32_t slot) const
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);
	}

	void OpenGLTexture3D::unbind() const
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

}
