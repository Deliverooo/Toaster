#include "toast_render/texture.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	TextureManager::TextureManager(RenderContext *p_render_ctx) : m_renderCtx(p_render_ctx)
	{
		TST_PERMA_ASSERT(m_renderCtx);

		m_textures.setDestructorUserData(this);
		m_textures.setDestructorFn(+[](Texture *p_data, void *p_user_data) -> void
		{
			TextureManager *ts{static_cast<TextureManager *>(p_user_data)};

			if (p_data->texture)
				gpu::frame::defferTextureDeletion(p_data->texture);

			if (p_data->shaderReadHeapSlot != UINT32_MAX)
				gpu::frame::defferTextureSlotFreeing(ts->m_renderCtx->getResourceHeap(), p_data->shaderReadHeapSlot);
			if (p_data->storageHeapSlot != UINT32_MAX)
				gpu::frame::defferTextureSlotFreeing(ts->m_renderCtx->getResourceHeap(), p_data->storageHeapSlot);

			for (const auto &slot: p_data->perMipStorageHeapSlots | std::views::values)
			{
				if (slot != UINT32_MAX)
					gpu::frame::defferTextureSlotFreeing(ts->m_renderCtx->getResourceHeap(), slot);
			}
		});
	}

	TextureManager::~TextureManager()
	{
		gpu::waitQueueIdle(gpu::EQueueType::eTransfer); // Make sure all the state trackers are ready to be destroyed
		gpu::upload::pollUploads();

		m_textures.clear();
	}

	auto TextureManager::registerTexture() -> TextureHandle
	{
		Texture temp_texture{};
		temp_texture.stateTracker = gpu::upload::registerStateTracker(1u); // Just the texture

		TextureHandle out_handle{m_textures.emplace(temp_texture)};

		gpu::upload::registerStateTrackerReadyCallback(temp_texture.stateTracker, [this, out_handle]() -> void
		{
			if (getTextureDesc(m_textures[out_handle].texture).mipCount > 1u)
				m_pendingMipmapGenerations.push_back(out_handle);
		});

		return out_handle;
	}

	auto TextureManager::createTexture(const gpu::TextureDesc &p_desc) -> TextureHandle
	{
		Texture texture_data{};

		texture_data.texture = gpu::createTexture(p_desc);

		if (p_desc.usage & gpu::ETextureUsageFlagBits::eSampled)
		{
			texture_data.shaderReadHeapSlot = gpu::allocTextureHeapSlot(m_renderCtx->getResourceHeap());
			gpu::writeTextureDescriptor(m_renderCtx->getResourceHeap(), texture_data.shaderReadHeapSlot, texture_data.texture, false);
		}

		if (p_desc.usage & gpu::ETextureUsageFlagBits::eStorage)
		{
			texture_data.storageHeapSlot = gpu::allocTextureHeapSlot(m_renderCtx->getResourceHeap());
			gpu::writeTextureDescriptor(m_renderCtx->getResourceHeap(), texture_data.storageHeapSlot, texture_data.texture, true);
		}

		texture_data.stateTracker = gpu::upload::registerStateTracker(1u); // Just the texture

		return m_textures.emplace(std::move(texture_data));
	}

	auto TextureManager::destroyTexture(TextureHandle p_handle) -> void
	{
		m_textures.destroy(p_handle);
	}

	auto TextureManager::createIntoTexture(TextureHandle p_handle, const gpu::TextureDesc &p_desc) -> void
	{
		Texture &texture{m_textures[p_handle]};

		texture.texture = gpu::createTexture(p_desc);

		if (p_desc.usage & gpu::ETextureUsageFlagBits::eSampled)
		{
			texture.shaderReadHeapSlot = gpu::allocTextureHeapSlot(m_renderCtx->getResourceHeap());
			gpu::writeTextureDescriptor(m_renderCtx->getResourceHeap(), texture.shaderReadHeapSlot, texture.texture, false);
		}
	}

	auto TextureManager::setData(TextureHandle p_handle, const void *p_data, uint64 p_size) -> void
	{
		Texture &              texture_data{m_textures[p_handle]};
		const gpu::TextureDesc secret_desc{gpu::getTextureDesc(texture_data.texture)};

		gpu::upload::TextureUploadDesc upload_desc{};
		upload_desc.dstTexture = texture_data.texture;
		upload_desc.data       = p_data;
		upload_desc.size       = p_size;
		upload_desc.layerCount = secret_desc.layerCount;
		upload_desc.baseLayer  = 0u;
		upload_desc.extent     = {0u, 0u, 0u}; // Use the desc
		upload_desc.mipLevel   = 0u;

		gpu::upload::uploadDataToTexture(upload_desc, texture_data.stateTracker);
	}

	auto TextureManager::getMipStorageHeapSlot(TextureHandle p_handle, uint32 p_mip) -> uint32
	{
		Texture &texture_data{m_textures[p_handle]};

		if (texture_data.perMipStorageHeapSlots.contains(p_mip))
			return texture_data.perMipStorageHeapSlots.at(p_mip);

		uint32 &slot{texture_data.perMipStorageHeapSlots[p_mip]};
		slot = gpu::allocTextureHeapSlot(m_renderCtx->getResourceHeap());
		gpu::writeTextureDescriptor(m_renderCtx->getResourceHeap(), slot, texture_data.texture, true, p_mip);

		return slot;
	}

	auto TextureManager::pollTextureUploads(gpu::CommandListHandle p_cmd) -> void
	{
		if (m_pendingMipmapGenerations.empty())
			return;

		for (auto handle: m_pendingMipmapGenerations)
		{
			Texture &texture{m_textures[handle]};
			gpu::generateMipmaps(p_cmd, texture.texture);
		}
		m_pendingMipmapGenerations.clear();
	}
}
