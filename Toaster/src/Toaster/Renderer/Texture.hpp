#pragma once
#include "Toaster/Core/Log.hpp"
#include <glm/glm.hpp>

namespace tst
{
	struct ColourRgba
	{
		int r{ 255 };
		int g{ 255 };
		int b{ 255 };
		int a{ 255 };
	};

	struct ColourFloat
	{
		float r{ 1.0f };
		float g{ 1.0f };
		float b{ 1.0f };
		float a{ 1.0f };
	};

	struct ColourHex
	{
		uint32_t rgba;
	};

	enum class TextureFormat
	{
		None = 0,
		RGB, RGBA,
	};

	enum class TextureWrapping
	{
		Repeat,
		ClampToEdge,
		ClampToBorder,
		MirroredRepeat,
	};

	enum class TextureFiltering
	{
		Nearest,
		Linear,
		NearestMipmapNearest,
		LinearMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapLinear,
	};

	struct TextureParams
	{
		TextureWrapping wrapS{ TextureWrapping::Repeat };
		TextureWrapping wrapT{ TextureWrapping::Repeat };

		TextureFiltering minFilter{ TextureFiltering::LinearMipmapLinear };
		TextureFiltering magFilter{ TextureFiltering::Linear };

		bool generateMipmaps{ true };
		bool flipX{ false };
	};

	class TST_API Texture
	{
	public:

		virtual ~Texture() {}

		virtual uint32_t getWidth()  const = 0;
		virtual uint32_t getHeight() const = 0;

		//virtual TextureFormat getFormat() const = 0;

		virtual void setData(void* data, size_t size) = 0;

		virtual void bind(uint32_t slot = 0) const = 0;
		virtual void unbind() const = 0;

		virtual uint32_t getId() const = 0;

		virtual void setParameters(const TextureParams& params) = 0;
		virtual void setWrapMode(const TextureWrapping wrapS, const TextureWrapping wrapT) = 0;
		virtual void setFilterMode(const TextureFiltering minFilter, const TextureFiltering magFilter) = 0;
		virtual const TextureParams& getParams() const = 0;

	};

	class TST_API Texture2D : public Texture
	{
	public:

		virtual ~Texture2D() {}

		static RefPtr<Texture2D> create(const std::string& filepath);
		static RefPtr<Texture2D> create(const ColourRgba& colour);
		static RefPtr<Texture2D> create(const ColourFloat& colour);
		static RefPtr<Texture2D> create(const uint32_t colour);
		static RefPtr<Texture2D> create(const uint32_t width, const uint32_t height);

		static RefPtr<Texture2D> create(const ColourRgba& colour, const TextureParams& params);
		static RefPtr<Texture2D> create(const std::string& filepath, const TextureParams& params);
		static RefPtr<Texture2D> create(const ColourFloat& colour, const TextureParams& params);
		static RefPtr<Texture2D> create(const uint32_t colour, const TextureParams& params);
		static RefPtr<Texture2D> create(const uint32_t width, const uint32_t height, const TextureParams& params);

		virtual bool operator==(const Texture2D& other) const = 0;
	};

	class TST_API Texture3D : public Texture
	{
	public:

		virtual ~Texture3D() {}

		static RefPtr<Texture3D> create(const std::vector<std::string>& texturePaths);
		static RefPtr<Texture3D> create(const ColourRgba& colour);
		static RefPtr<Texture3D> create(const ColourFloat& colour);
		static RefPtr<Texture3D> create(const uint32_t colour);
		static RefPtr<Texture3D> create(const uint32_t width, const uint32_t height);

		static RefPtr<Texture3D> create(const std::vector<std::string>& texturePaths, const TextureParams& params);
		static RefPtr<Texture3D> create(const ColourRgba& colour, const TextureParams& params);
		static RefPtr<Texture3D> create(const ColourFloat& colour, const TextureParams& params);
		static RefPtr<Texture3D> create(const uint32_t colour, const TextureParams& params);
		static RefPtr<Texture3D> create(const uint32_t width, const uint32_t height, const TextureParams& params);

		virtual void setFaceData(uint32_t faceIndex, const void* data, uint32_t width, uint32_t height) = 0;

		virtual bool operator==(const Texture3D& other) const = 0;
	};

	class TST_API SubTexture2D
	{
	public:
		~SubTexture2D() = default;

		// Constructors
		SubTexture2D(RefPtr<Texture2D> texture, const glm::vec2& min, const glm::vec2& max);

		// Static factory methods
		static RefPtr<SubTexture2D> create(RefPtr<Texture2D> texture, const glm::vec2& offset, const glm::vec2& size);
		static RefPtr<SubTexture2D> createFromPixels(RefPtr<Texture2D> texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		static RefPtr<SubTexture2D> createFromAtlas(RefPtr<Texture2D> texture, uint32_t spriteIndex, uint32_t spritesPerRow, uint32_t spriteWidth, uint32_t spriteHeight);
		static RefPtr<SubTexture2D> createFromGrid(RefPtr<Texture2D> texture, uint32_t gridX, uint32_t gridY, uint32_t cellWidth, uint32_t cellHeight, uint32_t spacing = 0);
		static RefPtr<SubTexture2D> createPixelPerfect(RefPtr<Texture2D> texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		// Accessors
		const glm::vec2* getTextureCoords() const { return m_textureCoordinates; }
		RefPtr<Texture2D> getBaseTexture() const { return m_texture; }

		// Enhanced getters with more information
		glm::vec2 getSize() const { return m_size; }
		glm::vec2 getOffset() const { return m_offset; }
		glm::vec2 getUVMin() const { return m_textureCoordinates[0]; }
		glm::vec2 getUVMax() const { return m_textureCoordinates[2]; }
		glm::vec2 getUVSize() const { return getUVMax() - getUVMin(); }

		// Utility methods
		float getAspectRatio() const { return m_size.x / m_size.y; }
		bool contains(const glm::vec2& uv) const;
		glm::vec2 transformUV(const glm::vec2& localUV) const;

		// Animation support
		RefPtr<SubTexture2D> getNeighbor(int deltaX, int deltaY, uint32_t spriteWidth, uint32_t spriteHeight) const;
		std::vector<RefPtr<SubTexture2D>> createAnimationFrames(uint32_t frameCount, uint32_t spriteWidth, uint32_t spriteHeight, bool horizontal = true) const;

		// Flipping and rotation
		RefPtr<SubTexture2D> createFlippedX() const;
		RefPtr<SubTexture2D> createFlippedY() const;
		RefPtr<SubTexture2D> createFlipped(bool flipX, bool flipY) const;
		RefPtr<SubTexture2D> createRotated90() const;

		// Comparison operators
		bool operator==(const SubTexture2D& other) const;
		bool operator!=(const SubTexture2D& other) const { return !(*this == other); }

	private:
		RefPtr<Texture2D> m_texture;
		glm::vec2 m_textureCoordinates[4];
		glm::vec2 m_size;      // Size in pixels
		glm::vec2 m_offset;    // Offset in pixels

		// Helper methods
		void calculateTextureCoordinates(const glm::vec2& min, const glm::vec2& max);
		void updateSizeAndOffset();
	};
}
