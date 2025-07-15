#include "tstpch.h"
#include "OpenGLTexture.hpp"

#include <stb_image.h>
#include <glad/glad.h>

namespace tst
{
	OpenGLTexture::OpenGLTexture(const std::string& filepath)
	{
        glGenTextures(1, &m_textureId);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;

        unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            TST_CORE_INFO("Successfully Read Texture File: {0}", filepath);
        }
        else
        {
            TST_CORE_INFO("Failed to Read Texture File: {0}", filepath);
        }
        stbi_image_free(data);
	}

	OpenGLTexture::~OpenGLTexture()
	{
        glDeleteTextures(1, &m_textureId);
	}

	void OpenGLTexture::bind() const
	{
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
	}

	void OpenGLTexture::unbind() const
	{
        glActiveTexture(0);
        glBindTexture(GL_TEXTURE_2D, 0);
	}
}
