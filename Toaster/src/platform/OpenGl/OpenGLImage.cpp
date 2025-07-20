#include "tstpch.h"

#include <glad/glad.h>
#include "OpenGLImage.hpp"
#include "stb_image.h"

namespace tst {

    namespace {
        GLenum toGLInternalFormat(ImageFormat format) {
            switch (format) {
            case ImageFormat::RGB:     return GL_RGB8;
            case ImageFormat::RGBA:    return GL_RGBA8;
            case ImageFormat::RGBA32F: return GL_RGBA32F;
            default:                   return GL_RGBA8;
            }
        }
        GLenum toGLFormat(ImageFormat format) {
            switch (format) {
            case ImageFormat::RGB:     return GL_RGB;
            case ImageFormat::RGBA:    return GL_RGBA;
            case ImageFormat::RGBA32F: return GL_RGBA;
            default:                   return GL_RGBA;
            }
        }
        GLenum toGLType(ImageFormat format) {
            switch (format) {
            case ImageFormat::RGBA32F: return GL_FLOAT;
            default:                   return GL_UNSIGNED_BYTE;
            }
        }
    }

    OpenGLImage::OpenGLImage(std::string_view path) {
        int width = 0, height = 0, channels = 0;
        void* data = nullptr;
        bool isHDR = stbi_is_hdr(path.data());

        if (isHDR) {
            data = stbi_loadf(path.data(), &width, &height, &channels, 4);
            m_format = ImageFormat::RGBA32F;
        }
        else {
            data = stbi_load(path.data(), &width, &height, &channels, 4);
            m_format = ImageFormat::RGBA;
        }

        if (!data) {
            m_width = m_height = 0;
            m_textureID = 0;
            m_format = ImageFormat::None;
            return;
        }

        m_width = static_cast<uint32_t>(width);
        m_height = static_cast<uint32_t>(height);

        allocate();
        setData(data);

        stbi_image_free(data);
    }

    OpenGLImage::OpenGLImage(uint32_t width, uint32_t height, ImageFormat format, const void* data)
        : m_width(width), m_height(height), m_format(format)
    {
        allocate();
        if (data)
            setData(data);
    }

    OpenGLImage::~OpenGLImage() {
        release();
    }

    OpenGLImage::OpenGLImage(OpenGLImage&& other) noexcept
        : m_width(other.m_width), m_height(other.m_height), m_textureID(other.m_textureID), m_format(other.m_format)
    {
        other.m_textureID = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_format = ImageFormat::None;
    }

    OpenGLImage& OpenGLImage::operator=(OpenGLImage&& other) noexcept {
        if (this != &other) {
            release();
            m_width = other.m_width;
            m_height = other.m_height;
            m_textureID = other.m_textureID;
            m_format = other.m_format;
            other.m_textureID = 0;
            other.m_width = 0;
            other.m_height = 0;
            other.m_format = ImageFormat::None;
        }
        return *this;
    }

    void OpenGLImage::allocate() {
        release();
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_2D, m_textureID);

        glTexImage2D(
            GL_TEXTURE_2D, 0,
            toGLInternalFormat(m_format),
            m_width, m_height, 0,
            toGLFormat(m_format),
            toGLType(m_format),
            nullptr
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLImage::release() {
        if (m_textureID) {
            glDeleteTextures(1, &m_textureID);
            m_textureID = 0;
        }
    }

    void OpenGLImage::setData(const void* data) {
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
            toGLFormat(m_format),
            toGLType(m_format),
            data
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLImage::resize(uint32_t width, uint32_t height) {
        if (m_width == width && m_height == height)
            return;
        m_width = width;
        m_height = height;
        allocate();
    }

    void OpenGLImage::bind()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
    }


}