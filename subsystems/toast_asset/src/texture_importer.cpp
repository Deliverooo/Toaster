#include "toast_asset/texture_importer.hpp"
#include <stb/stb_image.h>

namespace toaster::asset
{
	TextureImporter::TextureImporter(render::TextureManager *p_texture_manager) : m_textureManager(p_texture_manager)
	{
	}

	auto TextureImporter::importFromFile(const std::filesystem::path &p_path) -> render::TextureHandle
	{
		int32  width, height, num_channels;
		uint8 *data{stbi_load(p_path.string().c_str(), &width, &height, &num_channels, 4u)};

		gpu::TextureDesc texture_desc{};
		texture_desc.usage  = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
		texture_desc.format = gpu::EFormat::eR8G8B8A8Srgb;
		texture_desc.extent = {static_cast<uint32>(width), static_cast<uint32>(height), 1u};

		render::TextureHandle texture{m_textureManager->createTexture(texture_desc)};
		m_textureManager->setData(texture, data, width * height * sizeof(uint32));

		stbi_image_free(data);

		return texture;
	}
}
