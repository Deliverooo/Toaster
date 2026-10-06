#include "toast_asset/texture_importer.hpp"
#include <algorithm>
#include <cmath>
#include <stb/stb_image.h>

#include "toast_gpu/upload.hpp"

namespace toaster::asset
{
	auto loadTextureIntoBuffer(StringView p_path, gpu::EFormat &p_out_format, uint32 &p_out_width, uint32 &p_out_height) -> DataBuffer
	{
		DataBuffer image_data{};

		const bool is_srgb = (p_out_format == gpu::EFormat::eR8G8B8Srgb) || (p_out_format == gpu::EFormat::eR8G8B8A8Srgb);
		int32      width{0u};
		int32      height{0u};
		int32      num_channels{0u};

		if (stbi_is_hdr(p_path.data()))
		{
			const auto data{reinterpret_cast<uint8 *>(stbi_loadf(p_path.data(), &width, &height, &num_channels, 4))};
			if (!data)
				return DataBuffer{};
			TST_ASSERT_MSG(width != 0 && height != 0, "Bradar, wat is dis?");
			uint64 size{width * height * 4 * sizeof(float32)};
			image_data.allocate(size);
			image_data.write(data, size);

			p_out_format = gpu::EFormat::eR32G32B32A32Sfloat;

			stbi_image_free(data);
		}
		else
		{
			uint8 *data{stbi_load(p_path.data(), &width, &height, &num_channels, 4)};
			if (!data)
				return DataBuffer{};

			TST_ASSERT_MSG(width != 0 && height != 0, "Bradar, wat is dis?");
			const uint64 size{width * height * sizeof(uint32)};
			image_data.allocate(size);
			image_data.write(data, size);

			p_out_format = is_srgb ? gpu::EFormat::eR8G8B8A8Srgb : gpu::EFormat::eR8G8B8A8Unorm;

			stbi_image_free(data);
		}

		if (!image_data.data())
		{
			std::println("Failed to load image: {}", p_path);
			return {};
		}

		p_out_width  = width;
		p_out_height = height;
		return image_data;
	}

	auto asyncLoadTextureFromFile(render::TextureHandle p_dst_texture, render::TextureManager &p_texture_manager, const std::filesystem::path &p_path,
								  tf::Executor &        p_executor) -> void
	{
		p_executor.silent_async([p_dst_texture, &p_texture_manager, p_path, &p_executor]() -> void
		{
			tf::Taskflow graph{};

			tf::Task load_task{
				graph.emplace([p_dst_texture, &p_texture_manager, p_path]() -> void
				{
					gpu::TextureDesc texture_desc{};
					texture_desc.usage  = gpu::ETextureUsageFlagBits::eTransferSrc | gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
					texture_desc.format = gpu::EFormat::eR8G8B8A8Srgb;
					texture_desc.extent.z = 1u;

					DataBuffer image_data{loadTextureIntoBuffer(p_path.string(), texture_desc.format, texture_desc.extent.x, texture_desc.extent.y)};
					texture_desc.mipCount = static_cast<uint32>(std::floor(std::log2(std::max(texture_desc.extent.x, texture_desc.extent.y)))) + 1u;

					p_texture_manager.createIntoTexture(p_dst_texture, texture_desc);
					p_texture_manager.setData(p_dst_texture, image_data.data(), image_data.size());

					image_data.release();
				})
			};

			load_task.name("Load");

			p_executor.run(graph).wait();
		});
	}
}
