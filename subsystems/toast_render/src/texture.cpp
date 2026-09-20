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
			{
				gpu::upload::cancelTextureUpload(p_data->texture);
				gpu::frame::defferTextureDeletion(p_data->texture);
			}

			if (p_data->shaderReadHeapSlot != UINT32_MAX)
				gpu::frame::defferTextureSlotFreeing(ts->m_renderCtx->getResourceHeap(), p_data->shaderReadHeapSlot);
			if (p_data->storageHeapSlot != UINT32_MAX)
				gpu::frame::defferTextureSlotFreeing(ts->m_renderCtx->getResourceHeap(), p_data->storageHeapSlot);

			for (const auto &slot: p_data->perMipStorageHeapSlots | std::views::values)
			{
				if (slot != UINT32_MAX)
					gpu::frame::defferTextureSlotFreeing(ts->m_renderCtx->getResourceHeap(), slot);
			}

			p_data->state->store(ETextureState::eUnloaded);
		});
	}

	TextureManager::~TextureManager()
	{
		m_textures.clear();
	}

	auto TextureManager::registerTexture() -> TextureHandle
	{
		Texture temp_texture{};
		temp_texture.state = makeUnique<std::atomic<ETextureState> >(ETextureState::eUnloaded);

		return m_textures.emplace(std::move(temp_texture));
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

		texture_data.state = makeUnique<std::atomic<ETextureState> >(ETextureState::eUnloaded);

		return m_textures.emplace(std::move(texture_data));
	}

	auto TextureManager::destroyTexture(TextureHandle p_handle) -> void
	{
		{
			std::scoped_lock<std::mutex> lock{m_textureStateMutex};
			if (m_pendingTextureUploads.contains(p_handle))
				m_pendingTextureUploads.erase(p_handle);
		}

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
		upload_desc.layerCount          = secret_desc.layerCount;
		upload_desc.baseLayer           = 0u;
		upload_desc.extent              = {0u, 0u, 0u}; // Use the desc
		upload_desc.mipLevel            = 0u;
		texture_data.transferReadyToken = gpu::upload::uploadDataToTexture(texture_data.texture, p_data, p_size, upload_desc);

		texture_data.state->store(ETextureState::eUploadingToGPU);

		{
			std::scoped_lock<std::mutex> lock{m_textureStateMutex};
			m_pendingTextureUploads.insert(p_handle);
		}
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
		std::scoped_lock<std::mutex> lock{m_textureStateMutex};

		if (m_pendingTextureUploads.empty())
			return;

		uint64 transfer_value{gpu::getSemaphoreValue(gpu::frame::getTransferTimelineSemaphore())};
		for (auto it{m_pendingTextureUploads.begin()}; it != m_pendingTextureUploads.end();)
		{
			Texture &texture{m_textures[*it]};
			if (transfer_value >= texture.transferReadyToken)
			{
				if (gpu::getTextureDesc(texture.texture).mipCount > 1u)
					gpu::generateMipmaps(p_cmd, texture.texture);

				texture.state->store(ETextureState::eReady);

				it = m_pendingTextureUploads.erase(it);
			}
			else
				++it;
		}
	}
}
