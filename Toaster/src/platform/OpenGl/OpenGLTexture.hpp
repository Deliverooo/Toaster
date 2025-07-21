#pragma once
#include "Toaster/Renderer/Texture.hpp"
#include "Toaster/Core/Log.hpp"

namespace tst
{


	class TST_API OpenGLTexture2D : public Texture2D
	{
	public:

		virtual ~OpenGLTexture2D() override;

		OpenGLTexture2D(const std::string& filepath);
		OpenGLTexture2D(const ColourRgba& colour);
		OpenGLTexture2D(const ColourFloat& colour);
		OpenGLTexture2D(const uint32_t colour);

		OpenGLTexture2D(const uint32_t width, const uint32_t height);

		virtual uint32_t getWidth()  const override { return m_textureWidth; }
		virtual uint32_t getHeight() const override { return m_textureHeight; }


		virtual void bind(uint32_t slot = 0) const override;
		virtual void setData(void *data, size_t size) override;
		virtual void unbind() const override;

	private:

		std::optional<std::string> m_texturePath;

		uint32_t m_textureId;

		uint32_t m_textureWidth;
		uint32_t m_textureHeight;

		unsigned int m_dataFormat;
		unsigned int m_internalFormat;
	};

	class TST_API OpenGLTexture3D : public Texture3D
	{
	public:

		virtual ~OpenGLTexture3D() override;

		OpenGLTexture3D(const std::vector<std::string>& texturePaths);
		OpenGLTexture3D(const ColourRgba& colour);

		virtual void bind(uint32_t slot = 0)   const override;
		virtual void unbind() const override;

		virtual uint32_t getWidth()  const override { return m_textureWidth; }
		virtual uint32_t getHeight() const override { return m_textureHeight; }
		virtual void setData(void* data, size_t size) override{}

		//virtual TextureFormat getFormat() const override { return m_textureFormat; }

	private:	

		std::optional<std::vector<std::string>> m_texturePaths;

		uint32_t m_textureId;

		uint32_t m_textureWidth{10};
		uint32_t m_textureHeight{10};

		TextureFormat m_textureFormat{ TextureFormat::RGB };
	};

}
