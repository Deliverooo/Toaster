#include "toast_asset/texture_importer.hpp"
#include <stb/stb_image.h>

#include "toast_gpu/upload.hpp"

namespace toaster::asset
{
	TextureImporter::TextureImporter(render::TextureManager *p_texture_manager) : m_textureManager(p_texture_manager)
	{
	}

	TextureImporter::~TextureImporter()
	{
		m_terminationRequested.store(true);
		for (auto &import: m_pendingImports)
			import.join();
	}

	auto TextureImporter::asyncLoadTextureFromFile(render::TextureHandle p_dst_texture, const std::filesystem::path &p_path) -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};

		m_textureManager->setTextureState(p_dst_texture, render::ETextureState::eLoading);
		m_pendingImports.emplace_back([this, p_dst_texture, p_path]()-> void
		{
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

			m_textureManager->createIntoTexture(p_dst_texture, texture_desc);
			m_textureManager->setData(p_dst_texture, data, width * height * sizeof(uint32));

			stbi_image_free(data);
		});
	}

	auto TextureImporter::waitImports() -> void
	{
		std::scoped_lock<std::mutex> lock{m_mutex};

		for (auto &import: m_pendingImports)
			import.join();
		m_pendingImports.clear();
	}
}
