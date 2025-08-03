#include "tstpch.h"
#include "OpenGLTexture.hpp"

#include <stb_image.h>


namespace tst
{

	GLenum TextureUtils::toGLWrap(const TextureWrapping wrapping)
	{
		switch (wrapping)
		{
		case TextureWrapping::Repeat:			return GL_REPEAT;
		case TextureWrapping::ClampToEdge:		return GL_CLAMP_TO_EDGE;
		case TextureWrapping::ClampToBorder:	return GL_CLAMP_TO_BORDER;
		case TextureWrapping::MirroredRepeat:	return GL_MIRRORED_REPEAT;
		default: return GL_REPEAT;
		}
	}

	GLenum TextureUtils::toGLFilter(const TextureFiltering filtering)
	{
		switch (filtering)
		{
		case TextureFiltering::Nearest:					return GL_NEAREST;
		case TextureFiltering::Linear:					return GL_LINEAR;
		case TextureFiltering::NearestMipmapNearest:	return GL_NEAREST_MIPMAP_NEAREST;
		case TextureFiltering::LinearMipmapNearest:		return GL_LINEAR_MIPMAP_NEAREST;
		case TextureFiltering::NearestMipmapLinear:		return GL_NEAREST_MIPMAP_LINEAR;
		case TextureFiltering::LinearMipmapLinear:		return GL_LINEAR_MIPMAP_LINEAR;
		default: return GL_LINEAR;
		}
	}

	TextureWrapping TextureUtils::fromGLWrap(const GLenum wrapping)
	{
		switch (wrapping)
		{
		case GL_REPEAT:			 return TextureWrapping::Repeat;
		case GL_CLAMP_TO_EDGE:	 return TextureWrapping::ClampToEdge;
		case GL_CLAMP_TO_BORDER: return TextureWrapping::ClampToBorder;
		case GL_MIRRORED_REPEAT: return TextureWrapping::MirroredRepeat;
		default: return TextureWrapping::Repeat;
		}
	}

	TextureFiltering TextureUtils::fromGLFilter(const GLenum filtering)
	{
		switch (filtering)
		{
		case GL_NEAREST:				 return TextureFiltering::Nearest;
		case GL_LINEAR:					 return TextureFiltering::Linear;
		case GL_NEAREST_MIPMAP_NEAREST:	 return TextureFiltering::NearestMipmapNearest;
		case GL_LINEAR_MIPMAP_NEAREST:	 return TextureFiltering::LinearMipmapNearest;
		case GL_NEAREST_MIPMAP_LINEAR:	 return TextureFiltering::NearestMipmapLinear;
		case GL_LINEAR_MIPMAP_LINEAR:	 return TextureFiltering::LinearMipmapLinear;
		default: return TextureFiltering::Nearest;
		}
	}

	GLenum TextureUtils::toGLDataType(const TexturePixelDataType dataType)
	{
		switch (dataType)
		{
		case TexturePixelDataType::UnsignedByte: return GL_UNSIGNED_BYTE;
		case TexturePixelDataType::Byte: return GL_BYTE;
		case TexturePixelDataType::UnsignedShort: return GL_UNSIGNED_SHORT;
		case TexturePixelDataType::Short: return GL_SHORT;
		case TexturePixelDataType::UnsignedInt: return GL_UNSIGNED_INT;
		case TexturePixelDataType::Int: return GL_INT;
		case TexturePixelDataType::Float: return GL_FLOAT;
		default: return GL_UNSIGNED_BYTE;
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& filepath, const TextureParams& params) : m_texturePath(filepath), m_textureParameters(params)
	{
		TST_PROFILE_FN();

		int width, height, nrChannels;

		if (params.flipX)
		{
			stbi_set_flip_vertically_on_load(true);
		}
		unsigned char* data = nullptr;
		{
			TST_PROFILE_SCP("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string& filepath) : m_texturePath(filepath)");
			data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
		}

		if (!data)
		{
			TST_CORE_ERROR("Failed to load image -> {0}", filepath);

			m_textureWidth = 1;
			m_textureHeight = 1;

			m_dataFormat = GL_RGBA;
			m_internalFormat = GL_RGBA8;

			unsigned char invalidTextureData[4];
			invalidTextureData[0] = 255;
			invalidTextureData[1] = 0;
			invalidTextureData[2] = 220;
			invalidTextureData[3] = 255;

			createTexture(invalidTextureData); // The Magenta One :)

			return;
		}

		m_textureWidth = width;
		m_textureHeight = height;

		switch (nrChannels)
		{
		case 3: m_dataFormat = GL_RGB; m_internalFormat = GL_RGB8; break;
		case 4: m_dataFormat = GL_RGBA; m_internalFormat = GL_RGBA8; break;
		default: m_dataFormat = GL_RGB; m_internalFormat = GL_RGB8; break;
		}

		TST_ASSERT(m_dataFormat && m_internalFormat, "Invalid Texture Colour format");

		createTexture(data);

		stbi_image_free(data);
	}

	OpenGLTexture2D::OpenGLTexture2D(const ColourRgba& colour, const TextureParams& params) : m_textureWidth(1), m_textureHeight(1), m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		unsigned char data[4];
		data[0] = static_cast<unsigned char>(colour.r);
		data[1] = static_cast<unsigned char>(colour.g);
		data[2] = static_cast<unsigned char>(colour.b);
		data[3] = static_cast<unsigned char>(colour.a);


		createTexture(data);
	}

	OpenGLTexture2D::OpenGLTexture2D(const ColourFloat& colour, const TextureParams& params) : m_textureWidth(1), m_textureHeight(1), m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		unsigned char data[4];
		data[0] = static_cast<unsigned char>(colour.r * 255.0f);
		data[1] = static_cast<unsigned char>(colour.g * 255.0f);
		data[2] = static_cast<unsigned char>(colour.b * 255.0f);
		data[3] = static_cast<unsigned char>(colour.a * 255.0f);

		createTexture(data);
	}

	OpenGLTexture2D::OpenGLTexture2D(const uint32_t colour, const TextureParams& params) : m_textureWidth(1), m_textureHeight(1), m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		unsigned char data[4];
		data[0] = static_cast<unsigned char>((colour & 0xFF000000) >> 24); // R
		data[1] = static_cast<unsigned char>((colour & 0x00FF0000) >> 16); // G
		data[2] = static_cast<unsigned char>((colour & 0x0000FF00) >> 8);  // B
		data[3] = static_cast<unsigned char>((colour & 0x000000FF) >> 0);  // A

		createTexture(data);
	}

	OpenGLTexture2D::OpenGLTexture2D(const uint32_t width, const uint32_t height, const TextureParams& params) : m_textureWidth(width), m_textureHeight(height), m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		TST_ASSERT(m_dataFormat && m_internalFormat, "Invalid Texture Colour format");

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		applyParams();
	}

	void OpenGLTexture2D::invalidate()
	{
		if (m_textureId != 0)
		{
			glDeleteTextures(1, &m_textureId);
			m_textureId = 0;
		}
		m_textureWidth = 0;
		m_textureHeight = 0;
		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;
		TST_CORE_INFO("OpenGLTexture2D invalidated!");
	}

	void OpenGLTexture2D::setData(void* data)
	{
		TST_PROFILE_FN();

		if (!data) {
			TST_CORE_ERROR("OpenGLTexture2D::setData: Cannot set data with nullptr!");
			return;
		}

		if (m_textureId == 0) {
			TST_CORE_ERROR("OpenGLTexture2D::setData: Texture Id is not initialized!");
			return;
		}

		if (m_textureWidth == 0 || m_textureHeight == 0) {
			TST_CORE_ERROR("OpenGLTexture2D::setData: Invalid texture dimensions -> [{0} x {1}]", m_textureWidth, m_textureHeight);
			return;
		}



		glBindTexture(GL_TEXTURE_2D, m_textureId);

		// Check for binding errors
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			TST_CORE_ERROR("OpenGL Error after binding texture: {}", error);
			return;
		}

		TST_CORE_INFO("Setting texture data: [{0} x {1}], format: {2}, internal: {3}, type: {4}",
			m_textureWidth, m_textureHeight, m_dataFormat, m_internalFormat, TextureUtils::toGLDataType(m_textureParameters.pixelDataType));

		// Use glTexImage2D to set the data
		glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat,
			static_cast<int>(m_textureWidth), static_cast<int>(m_textureHeight), 0, m_dataFormat, TextureUtils::toGLDataType(m_textureParameters.pixelDataType), data);

		// Check for upload errors
		error = glGetError();
		if (error != GL_NO_ERROR) {
			TST_CORE_ERROR("OpenGL Error during texture upload: {} ({})", error,
				error == GL_INVALID_VALUE ? "GL_INVALID_VALUE" :
				error == GL_INVALID_ENUM ? "GL_INVALID_ENUM" :
				error == GL_INVALID_OPERATION ? "GL_INVALID_OPERATION" : "Unknown");

			// Log additional debug info
			TST_CORE_ERROR("Texture info - Width: {0}, Height: {1}, DataFormat: {2:x}, InternalFormat {3:x}, DataType: {4:x}",
				m_textureWidth, m_textureHeight, m_dataFormat, m_internalFormat,
				TextureUtils::toGLDataType(m_textureParameters.pixelDataType));
			return;
		}

		// Generate mipmaps if specified in parameters
		if (m_textureParameters.generateMipmaps) {
			glGenerateMipmap(GL_TEXTURE_2D);

			error = glGetError();
			if (error != GL_NO_ERROR) {
				TST_CORE_ERROR("OpenGL Error during mipmap generation: {0}", error);
			}
		}

		// Unbind texture to avoid conflicts
		glBindTexture(GL_TEXTURE_2D, 0);

		TST_CORE_INFO("Texture data uploaded successfully!");
	}


	OpenGLTexture2D::~OpenGLTexture2D()
	{
		TST_PROFILE_FN();

		glDeleteTextures(1, &m_textureId);
	}

	void OpenGLTexture2D::bind(uint32_t slot) const
	{
		TST_PROFILE_FN();

		glBindTextureUnit(slot, m_textureId);
	}

	void OpenGLTexture2D::unbind() const
	{
		TST_PROFILE_FN();

		glBindTextureUnit(0, m_textureId);

	}

	void OpenGLTexture2D::applyParams()
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TextureUtils::toGLWrap(m_textureParameters.wrapS));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TextureUtils::toGLWrap(m_textureParameters.wrapT));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureUtils::toGLFilter(m_textureParameters.minFilter));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureUtils::toGLFilter(m_textureParameters.magFilter));
	}

	void OpenGLTexture2D::createTexture(const void* data)
	{
		if (!data)
		{
			TST_ASSERT(false, "Failed to create texture, No data was provided!");
			return;
		}

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		applyParams();

		glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, static_cast<int>(m_textureWidth), static_cast<int>(m_textureHeight), 0, m_dataFormat, GL_UNSIGNED_BYTE, data);

		if (m_textureParameters.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	void OpenGLTexture2D::setParameters(const TextureParams& params)
	{
		m_textureParameters = params;
		glBindTexture(GL_TEXTURE_2D, m_textureId);

		applyParams();

		if (params.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}


	void OpenGLTexture2D::setWrapMode(const TextureWrapping wrapS, const TextureWrapping wrapT)
	{
		m_textureParameters.wrapS = wrapS;
		m_textureParameters.wrapT = wrapT;

		glBindTexture(GL_TEXTURE_2D, m_textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TextureUtils::toGLWrap(wrapS));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TextureUtils::toGLWrap(wrapT));
	}

	void OpenGLTexture2D::setFilterMode(const TextureFiltering minFilter, const TextureFiltering magFilter)
	{


		m_textureParameters.minFilter = minFilter;
		m_textureParameters.magFilter = magFilter;

		glBindTexture(GL_TEXTURE_2D, m_textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureUtils::toGLFilter(minFilter));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureUtils::toGLFilter(magFilter));
	}

	// ---------- OpenGL Texture 3D ------------

	OpenGLTexture3D::OpenGLTexture3D(const std::vector<std::string>& texturePaths, const TextureParams& params)
		: m_texturePaths(texturePaths), m_textureParameters(params)
	{
		TST_PROFILE_FN();

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		int width, height, nrChannels;

		for (uint32_t i = 0; i < texturePaths.size() && i < 6; i++)
		{
			unsigned char* data = nullptr;
			{
				TST_PROFILE_SCP("stbi_load - OpenGLTexture3D::OpenGLTexture3D(const std::vector<std::string>& texturePaths)");
				data = stbi_load(texturePaths[i].c_str(), &width, &height, &nrChannels, 0);
			}

			if (data)
			{

				switch (nrChannels)
				{
				case 3: m_dataFormat = GL_RGB; m_internalFormat = GL_RGB8; break;
				case 4: m_dataFormat = GL_RGBA; m_internalFormat = GL_RGBA8; break;
				default: m_dataFormat = GL_RGB; m_internalFormat = GL_RGB8; break;
				}

				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_internalFormat, width, height, 0, m_dataFormat, GL_UNSIGNED_BYTE, data);

				m_textureDimensions[i] = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

				stbi_image_free(data);
			}
			else
			{
				TST_CORE_ERROR("Cubemap Texture failed to load at path -> {0}", texturePaths[i]);

				unsigned char fallbackData[4] = { 255, 0, 220, 255 };
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackData);

				m_textureDimensions[i] = { 1, 1 };
			}
		}

		for (uint32_t i = texturePaths.size(); i < 6; i++)
		{
			unsigned char fallbackData[4] = { 255, 0, 220, 255 };
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackData);
			m_textureDimensions[i] = { 1, 1 };
		}

		applyParams();

		if (m_textureParameters.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
	}

	OpenGLTexture3D::OpenGLTexture3D(const ColourRgba& colour, const TextureParams& params) : m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		unsigned char data[4];
		data[0] = static_cast<unsigned char>(colour.r);
		data[1] = static_cast<unsigned char>(colour.g);
		data[2] = static_cast<unsigned char>(colour.b);
		data[3] = static_cast<unsigned char>(colour.a);

		for (uint32_t i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_internalFormat, 1, 1, 0, m_dataFormat, GL_UNSIGNED_BYTE, &data[0]);
			m_textureDimensions[i] = { 1, 1 };
		}

		applyParams();

		if (m_textureParameters.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}

	}

	OpenGLTexture3D::OpenGLTexture3D(const ColourFloat& colour, const TextureParams& params) : m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		unsigned char data[4];
		data[0] = static_cast<unsigned char>(colour.r * 255.0f);
		data[1] = static_cast<unsigned char>(colour.g * 255.0f);
		data[2] = static_cast<unsigned char>(colour.b * 255.0f);
		data[3] = static_cast<unsigned char>(colour.a * 255.0f);

		for (uint32_t i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_internalFormat, 1, 1, 0, m_dataFormat, GL_UNSIGNED_BYTE, &data[0]);
			m_textureDimensions[i] = { 1, 1 };
		}

		applyParams();

		if (m_textureParameters.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
	}

	OpenGLTexture3D::OpenGLTexture3D(const uint32_t colour, const TextureParams& params) : m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		unsigned char data[4];
		data[0] = static_cast<unsigned char>((colour & 0xFF000000) >> 24); // R
		data[1] = static_cast<unsigned char>((colour & 0x00FF0000) >> 16); // G
		data[2] = static_cast<unsigned char>((colour & 0x0000FF00) >> 8);  // B
		data[3] = static_cast<unsigned char>((colour & 0x000000FF) >> 0);  // A

		for (uint32_t i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_internalFormat, 1, 1, 0, m_dataFormat, GL_UNSIGNED_BYTE, &data[0]);
			m_textureDimensions[i] = { 1, 1 };
		}

		applyParams();

		if (m_textureParameters.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}

	}

	OpenGLTexture3D::OpenGLTexture3D(const uint32_t width, const uint32_t height, const TextureParams& params) : m_textureParameters(params)
	{
		TST_PROFILE_FN();

		m_dataFormat = GL_RGBA;
		m_internalFormat = GL_RGBA8;

		glGenTextures(1, &m_textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		for (uint32_t i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_internalFormat, width, height, 0, m_dataFormat, GL_UNSIGNED_BYTE, nullptr);
			m_textureDimensions[i] = { width, height };
		}

		applyParams();
	}


	OpenGLTexture3D::~OpenGLTexture3D()
	{
		TST_PROFILE_FN();

		glDeleteTextures(1, &m_textureId);
	}

	void OpenGLTexture3D::bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_textureId);
	}

	void OpenGLTexture3D::unbind() const
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void OpenGLTexture3D::applyParams()
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, TextureUtils::toGLWrap(m_textureParameters.wrapS));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, TextureUtils::toGLWrap(m_textureParameters.wrapT));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, TextureUtils::toGLWrap(m_textureParameters.wrapT));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, TextureUtils::toGLFilter(m_textureParameters.minFilter));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, TextureUtils::toGLFilter(m_textureParameters.magFilter));
	}


	void OpenGLTexture3D::setParameters(const TextureParams& params)
	{
		m_textureParameters = params;
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		applyParams();

		if (params.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
	}

	void OpenGLTexture3D::setWrapMode(const TextureWrapping wrapS, const TextureWrapping wrapT)
	{
		m_textureParameters.wrapS = wrapS;
		m_textureParameters.wrapT = wrapT;

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, TextureUtils::toGLWrap(wrapS));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, TextureUtils::toGLWrap(wrapT));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, TextureUtils::toGLWrap(wrapT));
	}

	void OpenGLTexture3D::setFilterMode(const TextureFiltering minFilter, const TextureFiltering magFilter)
	{
		m_textureParameters.minFilter = minFilter;
		m_textureParameters.magFilter = magFilter;

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, TextureUtils::toGLFilter(minFilter));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, TextureUtils::toGLFilter(magFilter));
	}

	void OpenGLTexture3D::setFaceData(uint32_t faceIndex, const void* data, uint32_t width, uint32_t height)
	{
		if (faceIndex >= 6)
		{
			TST_CORE_ERROR("Attempted to set cubemap face data with out of range index!");
			return;
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, m_internalFormat,
			width, height, 0, m_dataFormat, GL_UNSIGNED_BYTE, data);

		m_textureDimensions[faceIndex] = { width, height };

		if (m_textureParameters.generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
	}


}