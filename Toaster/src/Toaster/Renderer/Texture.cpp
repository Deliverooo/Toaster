#include "tstpch.h"

#include "Texture.hpp"
#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLTexture.hpp"

namespace tst
{


	//-------------------------------Texture 2D-------------------------------

	RefPtr<Texture2D> Texture2D::create(const std::string& filepath)
	{
		return create(filepath, TextureParams{});
	}

	RefPtr<Texture2D> Texture2D::create(const std::string& filepath, const TextureParams& params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(filepath, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const ColourRgba& colour)
	{
		return create(colour, TextureParams{});
	}

	RefPtr<Texture2D> Texture2D::create(const ColourRgba& colour, const TextureParams& params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(colour, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const ColourFloat& colour, const TextureParams& params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(colour, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const ColourFloat& colour)
	{
		return create(colour, TextureParams{});
	}

	RefPtr<Texture2D> Texture2D::create(const uint32_t colour, const TextureParams& params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(colour, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const uint32_t colour)
	{
		return create(colour, TextureParams{});
	}

	RefPtr<Texture2D> Texture2D::create(const uint32_t width, const uint32_t height, const TextureParams& params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture2D>(width, height, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture2D> Texture2D::create(const uint32_t width, const uint32_t height)
	{
		return create(width, height, TextureParams{});
	}

	//-------------------------------Texture 3D-------------------------------

	RefPtr<Texture3D> Texture3D::create(const std::vector<std::string>& texturePaths)
	{
		return create(texturePaths, TextureParams{});
	}

	RefPtr<Texture3D> Texture3D::create(const std::vector<std::string>& texturePaths, const TextureParams& params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(texturePaths, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture3D> Texture3D::create(const ColourRgba& colour)
	{
		return create(colour, TextureParams{});
	}

	RefPtr<Texture3D> Texture3D::create(const ColourRgba& colour, const TextureParams &params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(colour, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture3D> Texture3D::create(const ColourFloat& colour)
	{
		return create(colour, TextureParams{});
	}

	RefPtr<Texture3D> Texture3D::create(const ColourFloat& colour, const TextureParams &params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(colour, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture3D> Texture3D::create(const uint32_t colour)
	{
		return create(colour, TextureParams{});
	}

	RefPtr<Texture3D> Texture3D::create(const uint32_t colour, const TextureParams &params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(colour, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}

	RefPtr<Texture3D> Texture3D::create(const uint32_t width, const uint32_t height)
	{
		return create(width, height, TextureParams{});
	}

	RefPtr<Texture3D> Texture3D::create(const uint32_t width, const uint32_t height, const TextureParams &params)
	{
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:   { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
		case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLTexture3D>(width, height, params); }
		case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
		}

		TST_ASSERT(false, "Unknown Render API specified");
		return nullptr;
	}


	// ------------ Sub-Texture 2D ------------

	RefPtr<SubTexture2D> SubTexture2D::create(RefPtr<Texture2D> texture, const glm::vec2& offset, const glm::vec2& size)
	{
		float texWidth = static_cast<float>(texture->getWidth());
		float texHeight = static_cast<float>(texture->getHeight());

		// Calculate UV coordinates with pixel center offset
		glm::vec2 min = { (offset.x + 0.5f) / texWidth, (offset.y + 0.5f) / texHeight };
		glm::vec2 max = { (offset.x + size.x - 0.5f) / texWidth, (offset.y + size.y - 0.5f) / texHeight };

		return std::make_shared<SubTexture2D>(texture, min, max);
	}

	RefPtr<SubTexture2D> SubTexture2D::createFromPixels(RefPtr<Texture2D> texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		return create(texture, glm::vec2(static_cast<float>(x), static_cast<float>(y)), glm::vec2(static_cast<float>(width), static_cast<float>(height)));
	}

	RefPtr<SubTexture2D> SubTexture2D::createFromAtlas(RefPtr<Texture2D> texture, uint32_t spriteIndex, uint32_t spritesPerRow, uint32_t spriteWidth, uint32_t spriteHeight)
	{
		uint32_t x = (spriteIndex % spritesPerRow) * spriteWidth;
		uint32_t y = (spriteIndex / spritesPerRow) * spriteHeight;
		return createFromPixels(texture, x, y, spriteWidth, spriteHeight);
	}

	RefPtr<SubTexture2D> SubTexture2D::createFromGrid(RefPtr<Texture2D> texture, uint32_t gridX, uint32_t gridY, uint32_t cellWidth, uint32_t cellHeight, uint32_t spacing)
	{
		uint32_t x = gridX * (cellWidth + spacing);
		uint32_t y = gridY * (cellHeight + spacing);
		return createFromPixels(texture, x, y, cellWidth, cellHeight);
	}

	// Alternative pixel-perfect method without center offset
	RefPtr<SubTexture2D> SubTexture2D::createPixelPerfect(RefPtr<Texture2D> texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		float texWidth = static_cast<float>(texture->getWidth());
		float texHeight = static_cast<float>(texture->getHeight());

		// Direct pixel boundary mapping (no center offset)
		glm::vec2 min = { static_cast<float>(x) / texWidth, static_cast<float>(y) / texHeight };
		glm::vec2 max = { static_cast<float>(x + width) / texWidth, static_cast<float>(y + height) / texHeight };

		return std::make_shared<SubTexture2D>(texture, min, max);
	}

	SubTexture2D::SubTexture2D(RefPtr<Texture2D> texture, const glm::vec2& min, const glm::vec2& max)
		: m_texture(texture)
	{
		calculateTextureCoordinates(min, max);
		updateSizeAndOffset();
	}

	void SubTexture2D::calculateTextureCoordinates(const glm::vec2& min, const glm::vec2& max)
	{

		// Standard quad texture coordinates (bottom-left, bottom-right, top-right, top-left)
		// Ensure coordinates are clamped to valid range [0, 1]
		glm::vec2 clampedMin = glm::clamp(min, glm::vec2(0.0f), glm::vec2(1.0f));
		glm::vec2 clampedMax = glm::clamp(max, glm::vec2(0.0f), glm::vec2(1.0f));

		m_textureCoordinates[0] = { clampedMin.x, clampedMin.y };  // Bottom-left
		m_textureCoordinates[1] = { clampedMax.x, clampedMin.y };  // Bottom-right
		m_textureCoordinates[2] = { clampedMax.x, clampedMax.y };  // Top-right
		m_textureCoordinates[3] = { clampedMin.x, clampedMax.y };  // Top-left
	}

	void SubTexture2D::updateSizeAndOffset()
	{
		glm::vec2 uvMin = getUVMin();
		glm::vec2 uvMax = getUVMax();

		// Calculate precise pixel coordinates
		float texWidth = static_cast<float>(m_texture->getWidth());
		float texHeight = static_cast<float>(m_texture->getHeight());

		m_offset = glm::vec2(uvMin.x * texWidth, uvMin.y * texHeight);
		m_size = glm::vec2((uvMax.x - uvMin.x) * texWidth, (uvMax.y - uvMin.y) * texHeight);
	}

	bool SubTexture2D::contains(const glm::vec2& uv) const
	{
		glm::vec2 uvMin = getUVMin();
		glm::vec2 uvMax = getUVMax();
		return uv.x >= uvMin.x && uv.x <= uvMax.x && uv.y >= uvMin.y && uv.y <= uvMax.y;
	}

	glm::vec2 SubTexture2D::transformUV(const glm::vec2& localUV) const
	{
		glm::vec2 uvMin = getUVMin();
		glm::vec2 uvSize = getUVSize();
		return uvMin + localUV * uvSize;
	}

	RefPtr<SubTexture2D> SubTexture2D::getNeighbor(int deltaX, int deltaY, uint32_t spriteWidth, uint32_t spriteHeight) const
	{
		uint32_t newX = static_cast<uint32_t>(m_offset.x) + deltaX * spriteWidth;
		uint32_t newY = static_cast<uint32_t>(m_offset.y) + deltaY * spriteHeight;

		// Bounds checking
		if (newX + spriteWidth > m_texture->getWidth() || newY + spriteHeight > m_texture->getHeight()) {
			return nullptr;
		}

		return createFromPixels(m_texture, newX, newY, spriteWidth, spriteHeight);
	}

	std::vector<RefPtr<SubTexture2D>> SubTexture2D::createAnimationFrames(uint32_t frameCount, uint32_t spriteWidth, uint32_t spriteHeight, bool horizontal) const
	{
		std::vector<RefPtr<SubTexture2D>> frames;
		frames.reserve(frameCount);

		uint32_t startX = static_cast<uint32_t>(m_offset.x);
		uint32_t startY = static_cast<uint32_t>(m_offset.y);

		for (uint32_t i = 0; i < frameCount; ++i) {
			uint32_t frameX = horizontal ? startX + i * spriteWidth : startX;
			uint32_t frameY = horizontal ? startY : startY + i * spriteHeight;

			// Bounds checking
			if (frameX + spriteWidth <= m_texture->getWidth() && frameY + spriteHeight <= m_texture->getHeight()) {
				frames.push_back(createFromPixels(m_texture, frameX, frameY, spriteWidth, spriteHeight));
			}
		}

		return frames;
	}

	RefPtr<SubTexture2D> SubTexture2D::createFlippedX() const
	{
		auto flipped = std::make_shared<SubTexture2D>(m_texture, getUVMin(), getUVMax());

		// Swap X coordinates
		std::swap(flipped->m_textureCoordinates[0].x, flipped->m_textureCoordinates[1].x);
		std::swap(flipped->m_textureCoordinates[2].x, flipped->m_textureCoordinates[3].x);

		return flipped;
	}

	RefPtr<SubTexture2D> SubTexture2D::createFlippedY() const
	{
		auto flipped = std::make_shared<SubTexture2D>(m_texture, getUVMin(), getUVMax());

		// Swap Y coordinates
		std::swap(flipped->m_textureCoordinates[0].y, flipped->m_textureCoordinates[3].y);
		std::swap(flipped->m_textureCoordinates[1].y, flipped->m_textureCoordinates[2].y);

		return flipped;
	}

	RefPtr<SubTexture2D> SubTexture2D::createFlipped(bool flipX, bool flipY) const
	{
		auto result = std::make_shared<SubTexture2D>(m_texture, getUVMin(), getUVMax());

		if (flipX) {
			std::swap(result->m_textureCoordinates[0].x, result->m_textureCoordinates[1].x);
			std::swap(result->m_textureCoordinates[2].x, result->m_textureCoordinates[3].x);
		}

		if (flipY) {
			std::swap(result->m_textureCoordinates[0].y, result->m_textureCoordinates[3].y);
			std::swap(result->m_textureCoordinates[1].y, result->m_textureCoordinates[2].y);
		}

		return result;
	}

	RefPtr<SubTexture2D> SubTexture2D::createRotated90() const
	{
		auto rotated = std::make_shared<SubTexture2D>(m_texture, getUVMin(), getUVMax());

		// Rotate coordinates 90 degrees clockwise
		glm::vec2 temp = rotated->m_textureCoordinates[0];
		rotated->m_textureCoordinates[0] = rotated->m_textureCoordinates[3];
		rotated->m_textureCoordinates[3] = rotated->m_textureCoordinates[2];
		rotated->m_textureCoordinates[2] = rotated->m_textureCoordinates[1];
		rotated->m_textureCoordinates[1] = temp;

		return rotated;
	}

	bool SubTexture2D::operator==(const SubTexture2D& other) const
	{
		if (m_texture != other.m_texture) { return false; }

		for (int i = 0; i < 4; ++i) {
			if (glm::distance(m_textureCoordinates[i], other.m_textureCoordinates[i]) > 0.0001f) {
				return false;
			}
		}

		return true;
	}
}
