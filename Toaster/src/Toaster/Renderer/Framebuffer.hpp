#pragma once
#include "Texture.hpp"

namespace tst
{
	enum class FramebufferTextureFormat : uint8_t
	{
		None = 0,
		RGBA8,
		RGBA4,
		RGB8,
		RGBA16F,
		RGB16F,
		RGBA32F,
		RGB32F,
		D32F_S8,
		D24S8,
		RED_INTEGER,
	};

	enum class FramebufferTextureWrapping : uint8_t
	{
		Repeat,
		ClampToEdge,
		ClampToBorder,
		MirroredRepeat,
	};

	enum class FramebufferTextureFiltering : uint8_t
	{
		Nearest,
		Linear,
		NearestMipmapNearest,
		LinearMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapLinear,
	};

	enum class FramebufferPixelDataType : uint8_t
	{
		UnsignedByte,
		Byte,
		UnsignedShort,
		Short,
		UnsignedInt,
		Int,
		Float,
	};

	enum class FramebufferAttachmentType : uint8_t
	{
		None = 0,
		Colour,
		Depth,
		DepthStencil,
	};

	struct FramebufferTextureCreateInfo
	{
		FramebufferTextureCreateInfo() = default;
		FramebufferTextureCreateInfo(FramebufferTextureFormat format) : format(format) {}
		FramebufferTextureFormat	format			{ FramebufferTextureFormat::None };
		FramebufferTextureWrapping	wrapS			{ FramebufferTextureWrapping::Repeat };
		FramebufferTextureWrapping	wrapT			{ FramebufferTextureWrapping::Repeat };
		FramebufferTextureFiltering minFilter		{ FramebufferTextureFiltering::LinearMipmapLinear };
		FramebufferTextureFiltering magFilter		{ FramebufferTextureFiltering::Linear };
		FramebufferPixelDataType	pixelDataType	{ FramebufferPixelDataType::UnsignedByte };

	};

	struct FramebufferAttachmentCreateInfo
	{
		FramebufferAttachmentCreateInfo() = default;
		FramebufferAttachmentCreateInfo(std::initializer_list<FramebufferTextureCreateInfo> textures)
			: textures(textures) {}

		std::vector<FramebufferTextureCreateInfo> textures;
	};

	struct FramebufferCreateInfo
	{
		FramebufferAttachmentCreateInfo attachments;
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t samples{ 1 };
	};

	class TST_API Framebuffer
	{
	public:
		virtual ~Framebuffer(){};

		static RefPtr<Framebuffer> create(const FramebufferCreateInfo& createInfo);

		virtual const FramebufferCreateInfo& getInfo() const = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual int readPixel(uint32_t attachmentIndex, int x, int y) = 0;


		virtual void clearAttachment(uint32_t attachmentIndex, int value) = 0;

		virtual const uint32_t &getColourAttachmentId(uint32_t index = 0) const = 0;
		virtual const uint32_t &getDepthAttachmentId() const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
	};
}
