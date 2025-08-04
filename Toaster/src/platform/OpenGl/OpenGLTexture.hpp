#pragma once
#include "Toaster/Renderer/Texture.hpp"
#include "Toaster/Core/Log.hpp"
#include <glad/glad.h>

namespace tst
{

	// Texture utility class
	// I suppose it could have just been a namespace
	class TextureUtils
	{
	public:

		TextureUtils operator=(TextureUtils& other) = delete;

		static GLenum toGLWrap(const TextureWrapping wrapping);
		static GLenum toGLFilter(const TextureFiltering filtering);

		static TextureWrapping fromGLWrap(const GLenum wrapping);
		static TextureFiltering fromGLFilter(const GLenum filtering);

		static GLenum toGLDataType(const TexturePixelDataType dataType);
	};


	class TST_API OpenGLTexture2D : public Texture2D
	{
	public:

		virtual ~OpenGLTexture2D() override;

		OpenGLTexture2D(const std::string& filepath, const TextureParams &params = {});
		OpenGLTexture2D(const ColourRgba& colour, const TextureParams& params = {});
		OpenGLTexture2D(const ColourFloat& colour, const TextureParams& params = {});
		OpenGLTexture2D(const uint32_t colour, const TextureParams& params = {});
		OpenGLTexture2D(const uint32_t width, const uint32_t height, const TextureParams& params = {});

		virtual uint32_t getWidth()  const override { return m_textureWidth; }
		virtual uint32_t getHeight() const override { return m_textureHeight; }

		virtual void bind(uint32_t slot = 0) const override;
		virtual void setData(void *data) override;
		virtual void unbind() const override;

		virtual uint32_t getId() const override { return m_textureId; }

		virtual void  setParameters(const TextureParams &params) override;
		virtual void  setWrapMode(const TextureWrapping wrapS, const TextureWrapping wrapT) override;
		virtual void  setFilterMode(const TextureFiltering minFilter, const TextureFiltering magFilter) override;
		virtual const TextureParams& getParams() const override { return m_textureParameters; }

		//virtual void setFlipX(bool flipX) override;
		//virtual void setFlipY(bool flipY) override;

		virtual std::optional<std::string> getPath() const override { return m_texturePath; }
		virtual bool operator==(const Texture2D& other) const override { return (m_textureId == ((OpenGLTexture2D&)other).m_textureId); }

	private:

		void applyParams();
		void createTexture(const void* data = nullptr);
		void invalidate();

		uint8_t* decodeWebp(const std::string& filePath);

		std::optional<std::string> m_texturePath;
		uint32_t m_textureId;
		uint32_t m_textureWidth;
		uint32_t m_textureHeight;
		int m_dataFormat;
		int m_internalFormat;

		TextureParams m_textureParameters;
	};

	class TST_API OpenGLTexture3D : public Texture3D
	{
	public:

		virtual ~OpenGLTexture3D() override;

		OpenGLTexture3D(const std::vector<std::string>& texturePaths, const TextureParams& params = {});
		OpenGLTexture3D(const ColourRgba& colour, const TextureParams& params = {});
		OpenGLTexture3D(const ColourFloat& colour, const TextureParams& params = {});
		OpenGLTexture3D(const uint32_t colour, const TextureParams& params = {});
		OpenGLTexture3D(const uint32_t width, const uint32_t height, const TextureParams& params = {});

		virtual void bind(uint32_t slot = 0)   const override;
		virtual void unbind() const override;

		virtual uint32_t getWidth()  const override { return m_textureDimensions[0].first; }
		virtual uint32_t getHeight() const override { return m_textureDimensions[0].second; }

		std::pair<uint32_t, uint32_t> getFaceDimensions(uint32_t faceIndex) const { return ((faceIndex < 6) ? m_textureDimensions[faceIndex] : std::pair<uint32_t, uint32_t>(0, 0)); }

		virtual void setData(void* data) override{}

		virtual void setFaceData(uint32_t faceIndex, const void* data, uint32_t width, uint32_t height) override;

		virtual uint32_t getId() const override { return m_textureId; }

		virtual void  setParameters(const TextureParams& params) override;
		virtual void  setWrapMode(const TextureWrapping wrapS, const TextureWrapping wrapT) override;
		virtual void  setFilterMode(const TextureFiltering minFilter, const TextureFiltering magFilter) override;
		virtual const TextureParams& getParams() const override { return m_textureParameters; }

		//virtual void setFlipX(bool flipX) override;
		//virtual void setFlipY(bool flipY) override;

		virtual bool operator==(const Texture3D& other) const override { return (m_textureId == ((OpenGLTexture3D&)other).m_textureId); }

	private:	

		void applyParams();

		std::optional<std::vector<std::string>> m_texturePaths;
		uint32_t m_textureId;

		std::array<std::pair<uint32_t, uint32_t>, 6> m_textureDimensions;
		uint32_t m_textureWidth{1};
		uint32_t m_textureHeight{1};
		int m_dataFormat;
		int m_internalFormat;

		TextureParams m_textureParameters;
	};


	
}
