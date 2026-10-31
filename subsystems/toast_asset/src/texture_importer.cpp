#include "toast_asset/texture_importer.hpp"

#include <cmath>
#include <print>
#include <stb/stb_image.h>

#include "toast_gpu/api.hpp"
#include "toast_gpu/upload.hpp"
#include "toast_render/render_context.hpp"

#undef min
#undef max

namespace toaster::ast
{
	auto texture_load_into_buffer(StringView p_path, gpu::EFormat &p_out_format, uint32 &p_out_width, uint32 &p_out_height) -> DataBuffer
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

	auto texture_load_from_file(const rnd::RenderContext *p_render_ctx, gpu::TextureHandle p_dst_texture, uint32 p_dst_heap_slot, gpu::StateTrackerHandle p_dst_state_tracker,
								const std::filesystem::path &p_path, gpu::EFormat p_desired_format) -> void
	{
		gpu::TextureDesc texture_desc{};
		texture_desc.usage    = gpu::ETextureUsageFlagBits::eTransferSrc | gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eSampled;
		texture_desc.format   = p_desired_format;
		texture_desc.extent.z = 1u;

		DataBuffer image_data{texture_load_into_buffer(p_path.string(), texture_desc.format, texture_desc.extent.x, texture_desc.extent.y)};
		texture_desc.mipCount = static_cast<uint32>(std::floor(std::log2(std::max(texture_desc.extent.x, texture_desc.extent.y)))) + 1u;

		gpu::GPUContext *                       gpu_ctx{p_render_ctx->gpuCtx};
		gpu::UploadContext *                    upload_ctx{p_render_ctx->uploadCtx};
		const gpu::ResourceDescriptorHeapHandle resource_heap{p_render_ctx->resourceHeap};

		gpu::resource_descriptor_heap_write_texture(gpu_ctx, resource_heap, p_dst_heap_slot, p_dst_texture, false);
		gpu::texture_create_into(gpu_ctx, p_dst_texture, texture_desc);

		gpu::BufferTextureCopyRegion copy_region{};
		copy_region.bufferOffset       = 0ull;
		copy_region.textureOffset      = {0, 0, 0};
		copy_region.textureExtent      = {0u, 0u, 0u};
		copy_region.textureSubresource = gpu::TextureSubresource{0u, 0u, 1u};

		gpu::TextureUploadDesc upload_desc{};
		upload_desc.uploadRegions = copy_region;
		upload_desc.data          = image_data.data();
		upload_desc.size          = image_data.size();
		gpu::upload_data_to_texture(upload_ctx, p_dst_texture, upload_desc, p_dst_state_tracker);

		image_data.release();
	}
}
