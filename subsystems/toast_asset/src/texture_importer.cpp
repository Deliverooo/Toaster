#include "toast_asset/texture_importer.hpp"
#include <algorithm>
#include <cmath>
#include <stb/stb_image.h>

#include "toast_gpu/upload.hpp"

namespace toaster::asset
{
	auto asyncLoadTextureFromFile(render::TextureHandle p_dst_texture, render::TextureManager &p_texture_manager, const std::filesystem::path &p_path,
								  tf::Executor &        p_executor) -> void
	{
		p_executor.silent_async([p_dst_texture, &p_texture_manager, p_path, &p_executor]() -> void
		{
			tf::Taskflow graph{};

			tf::Task load_task{
				graph.emplace([p_dst_texture, &p_texture_manager, p_path]() -> void
				{
					int32  width, height, num_channels;
					uint8 *data{stbi_load(p_path.string().c_str(), &width, &height, &num_channels, 4u)};
					if (!data)
						return;

					gpu::TextureDesc texture_desc{};
					texture_desc.usage    = gpu::ETextureUsageFlagBits::eTransferSrc | gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
					texture_desc.format   = gpu::EFormat::eR8G8B8A8Srgb;
					texture_desc.extent   = {static_cast<uint32>(width), static_cast<uint32>(height), 1u};
					texture_desc.mipCount = static_cast<uint32>(std::floor(std::log2(std::max(static_cast<uint32>(width), static_cast<uint32>(height))))) + 1u;

					p_texture_manager.createIntoTexture(p_dst_texture, texture_desc);
					p_texture_manager.setData(p_dst_texture, data, width * height * sizeof(uint32));

					stbi_image_free(data);
				})
			};

			load_task.name("Load");

			p_executor.run(graph).wait();
		});
	}
}
