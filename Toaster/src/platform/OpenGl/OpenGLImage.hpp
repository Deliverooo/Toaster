#pragma once

#include <string>
#include <cstdint>

namespace tst {

    enum class ImageFormat
    {
        None = 0,
        RGB,
        RGBA,
        RGBA32F
    };

    class TST_API OpenGLImage
    {
    public:
        OpenGLImage(std::string_view path);
        OpenGLImage(uint32_t width, uint32_t height, ImageFormat format, const void* data = nullptr);
        ~OpenGLImage();

        void setData(const void* data);

        uint32_t getTextureID() const { return m_textureID; }
        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        ImageFormat getFormat() const { return m_format; }

        void resize(uint32_t width, uint32_t height);

        void bind();

        // Non-copyable, movable
        OpenGLImage(const OpenGLImage&) = delete;
        OpenGLImage& operator=(const OpenGLImage&) = delete;
        OpenGLImage(OpenGLImage&& other) noexcept;
        OpenGLImage& operator=(OpenGLImage&& other) noexcept;

    private:
        void allocate();
        void release();

        uint32_t m_width = 0, m_height = 0;
        uint32_t m_textureID = 0;
        ImageFormat m_format = ImageFormat::None;
    };

}