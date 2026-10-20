#include "toast_render/texture.hpp"

#include <ranges>

#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::rnd
{
	struct TextureManager
	{
		RenderContext *rndCtx{nullptr};

		Pool<Texture>              textures;
		std::vector<TextureHandle> pendingMipmapGenerations;
	};

	auto texture_mngr_create(RenderContext *p_render_ctx) -> TextureManager *
	{
		auto mngr{new TextureManager{}};

		mngr->rndCtx = p_render_ctx;

		mngr->textures.setDestructorUserData(mngr);
		mngr->textures.setDestructorFn(+[](Texture *p_data, void *p_user_data) -> void
		{
			auto               ts{static_cast<TextureManager *>(p_user_data)};
			gpu::FrameContext *frame_ctx{ts->rndCtx->frameCtx};

			if (p_data->texture)
				gpu::defer_texture_deletion(frame_ctx, p_data->texture);

			if (p_data->shaderReadHeapSlot != UINT32_MAX)
				gpu::defer_texture_heap_slot_freeing(frame_ctx, ts->rndCtx->resourceHeap, p_data->shaderReadHeapSlot);
			if (p_data->storageHeapSlot != UINT32_MAX)
				gpu::defer_texture_heap_slot_freeing(frame_ctx, ts->rndCtx->resourceHeap, p_data->storageHeapSlot);

			for (const auto &slot: p_data->perMipStorageHeapSlots | std::views::values)
			{
				if (slot != UINT32_MAX)
					gpu::defer_texture_heap_slot_freeing(frame_ctx, ts->rndCtx->resourceHeap, slot);
			}
		});

		return mngr;
	}

	auto texture_mngr_destroy(TextureManager *p_mngr) -> void
	{
		gpu::wait_queue_idle(p_mngr->rndCtx->gpuCtx, gpu::EQueueType::eTransfer); // Make sure all the state trackers are ready to be destroyed
		gpu::poll_uploads(p_mngr->rndCtx->uploadCtx);

		p_mngr->textures.clear();
		p_mngr->rndCtx = nullptr;
		p_mngr->pendingMipmapGenerations.clear();

		delete p_mngr;
	}

	auto texture_mngr_poll_uploads(TextureManager *p_mngr, gpu::CommandListHandle p_cmd) -> void
	{
		if (p_mngr->pendingMipmapGenerations.empty())
			return;

		for (const auto handle: p_mngr->pendingMipmapGenerations)
		{
			Texture &texture{p_mngr->textures[handle]};
			gpu::cmd_generate_mipmaps(p_mngr->rndCtx->gpuCtx, p_cmd, texture.texture);
		}
		p_mngr->pendingMipmapGenerations.clear();
	}

	auto texture_register(TextureManager *p_mngr, uint32 p_state_tracker_expected_subresources) -> TextureHandle
	{
		Texture temp_texture{};

		if (p_state_tracker_expected_subresources > 0u)
			temp_texture.stateTracker = gpu::state_tracker_register(p_mngr->rndCtx->uploadCtx, p_state_tracker_expected_subresources);

		TextureHandle out_handle{p_mngr->textures.emplace(temp_texture)};

		gpu::state_tracker_register_ready_callback(p_mngr->rndCtx->uploadCtx, temp_texture.stateTracker, [p_mngr, out_handle]() -> void
		{
			if (gpu::texture_get_desc(p_mngr->rndCtx->gpuCtx, p_mngr->textures[out_handle].texture).mipCount > 1u)
				p_mngr->pendingMipmapGenerations.push_back(out_handle);
		});

		return out_handle;
	}

	auto texture_create(TextureManager *p_mngr, const gpu::TextureDesc &p_desc, uint32 p_state_tracker_expected_subresources) -> TextureHandle
	{
		Texture texture_data{};

		texture_data.texture = gpu::texture_create(p_mngr->rndCtx->gpuCtx, p_desc);

		if (p_desc.usage & gpu::ETextureUsageFlagBits::eSampled)
		{
			texture_data.shaderReadHeapSlot = gpu::resource_descriptor_heap_alloc_texture_slot(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap);
			gpu::resource_descriptor_heap_write_texture(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap, texture_data.shaderReadHeapSlot, texture_data.texture,
														false);
		}

		if (p_desc.usage & gpu::ETextureUsageFlagBits::eStorage)
		{
			texture_data.storageHeapSlot = gpu::resource_descriptor_heap_alloc_texture_slot(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap);
			gpu::resource_descriptor_heap_write_texture(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap, texture_data.storageHeapSlot, texture_data.texture, true);
		}

		if (p_state_tracker_expected_subresources > 0u)
			texture_data.stateTracker = gpu::state_tracker_register(p_mngr->rndCtx->uploadCtx, p_state_tracker_expected_subresources);

		return p_mngr->textures.emplace(std::move(texture_data));
	}

	auto texture_create_into(TextureManager *p_mngr, TextureHandle p_handle, const gpu::TextureDesc &p_desc) -> void
	{
		Texture &texture{p_mngr->textures[p_handle]};

		texture.texture = gpu::texture_create(p_mngr->rndCtx->gpuCtx, p_desc);

		if (p_desc.usage & gpu::ETextureUsageFlagBits::eSampled)
		{
			texture.shaderReadHeapSlot = gpu::resource_descriptor_heap_alloc_texture_slot(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap);
			gpu::resource_descriptor_heap_write_texture(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap, texture.shaderReadHeapSlot, texture.texture, false);
		}
	}

	auto texture_destroy(TextureManager *p_mngr, TextureHandle p_texture) -> void
	{
		p_mngr->textures.destroy(p_texture);
	}

	auto texture_set_data(TextureManager *p_mngr, TextureHandle p_texture, const void *p_data, uint64 p_size) -> void
	{
		Texture &              texture_data{p_mngr->textures[p_texture]};
		const gpu::TextureDesc secret_desc{gpu::texture_get_desc(p_mngr->rndCtx->gpuCtx, texture_data.texture)};

		gpu::TextureUploadDesc upload_desc{};
		upload_desc.dstTexture = texture_data.texture;
		upload_desc.data       = p_data;
		upload_desc.size       = p_size;
		upload_desc.layerCount = secret_desc.layerCount;
		upload_desc.baseLayer  = 0u;
		upload_desc.extent     = {0u, 0u, 0u}; // Use the desc
		upload_desc.mipLevel   = 0u;

		gpu::upload_data_to_texture(p_mngr->rndCtx->uploadCtx, upload_desc, texture_data.stateTracker);
	}

	auto texture_wait_for_upload(TextureManager *p_mngr, TextureHandle p_texture) -> void
	{
		const Texture &texture_data{p_mngr->textures[p_texture]};
		gpu::state_tracker_wait_for(p_mngr->rndCtx->uploadCtx, texture_data.stateTracker);
	}

	auto texture_get(TextureManager *p_mngr, TextureHandle p_texture) -> Texture *
	{
		Texture &texture_data{p_mngr->textures[p_texture]};
		return &texture_data;
	}

	auto texture_try_get(TextureManager *p_mngr, TextureHandle p_texture) -> Texture *
	{
		Texture *texture_data{p_mngr->textures.tryGet(p_texture)};
		return texture_data;
	}

	auto texture_get_mip_storage_heap_slot(TextureManager *p_mngr, TextureHandle p_texture, uint32 p_mip) -> uint32
	{
		Texture &texture_data{p_mngr->textures[p_texture]};

		if (texture_data.perMipStorageHeapSlots.contains(p_mip))
			return texture_data.perMipStorageHeapSlots.at(p_mip);

		uint32 &slot{texture_data.perMipStorageHeapSlots[p_mip]};
		slot = gpu::resource_descriptor_heap_alloc_texture_slot(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap);
		gpu::resource_descriptor_heap_write_texture(p_mngr->rndCtx->gpuCtx, p_mngr->rndCtx->resourceHeap, slot, texture_data.texture, true, p_mip);

		return slot;
	}
}
