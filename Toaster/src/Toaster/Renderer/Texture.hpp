#pragma once

namespace tst
{
	struct ColourRgba
	{
		int r{255};
		int g{255};
		int b{255};
		int a{255};
	};

	struct ColourFloat
	{
		float r{1.0f};
		float g{1.0f};
		float b{1.0f};
		float a{1.0f};
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

	class TST_API Texture
	{
	public:

		virtual ~Texture() {}

		virtual uint32_t getWidth()  const = 0;
		virtual uint32_t getHeight() const = 0;

		virtual TextureFormat getFormat() const = 0;
			
		virtual void bind(uint32_t slot = 0) const = 0;
		virtual void unbind() const = 0;
	};

	class TST_API Texture2D : public Texture
	{
	public:

		virtual ~Texture2D() {}

		static RefPtr<Texture2D> create(const std::string& filepath);
		static RefPtr<Texture2D> create(const ColourRgba &colour);
		static RefPtr<Texture2D> create(const ColourFloat &colour);
		static RefPtr<Texture2D> create(const uint32_t colour);
	};

	class TST_API Texture3D : public Texture
	{
	public:

		virtual ~Texture3D() {}

		static RefPtr<Texture3D> create(const std::vector<std::string>& texturePaths);
		static RefPtr<Texture3D> create(const ColourRgba &colour);


	};

	
}
