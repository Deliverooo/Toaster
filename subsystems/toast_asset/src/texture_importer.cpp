#include "toast_asset/texture_importer.hpp"
#include <stb/stb_image.h>

namespace toaster::asset
{
	TextureImporter::~TextureImporter()
	{
		m_terminationRequested.store(true);
		for (auto &import: m_pendingImports)
			import.join();
	}

	auto TextureImporter::asyncLoadTextureFromFile(render::TextureManager *     p_texture_manager, render::TextureHandle p_dst_texture,
												   const std::filesystem::path &p_path) -> void
	{
		m_pendingImports.emplace_back([this, p_texture_manager, p_dst_texture, p_path]()-> void
		{
			auto &gpu_texture{p_texture_manager->getTexture(p_dst_texture)};

			if (m_terminationRequested.load())
				return;
			gpu_texture.state->store(render::ETextureState::eLoading);
			int32  width, height, num_channels;
			uint8 *data{stbi_load(p_path.string().c_str(), &width, &height, &num_channels, 4u)};
			if (m_terminationRequested.load())
			{
				stbi_image_free(data);
				return;
			}

			gpu::TextureDesc texture_desc{};
			texture_desc.usage  = gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
			texture_desc.format = gpu::EFormat::eR8G8B8A8Srgb;
			texture_desc.extent = {static_cast<uint32>(width), static_cast<uint32>(height), 1u};

			p_texture_manager->createIntoTexture(p_dst_texture, texture_desc);
			p_texture_manager->setData(p_dst_texture, data, width * height * sizeof(uint32));

			stbi_image_free(data);
		});
	}
}
