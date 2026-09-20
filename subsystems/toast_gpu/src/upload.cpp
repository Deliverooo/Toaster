#include "toast_gpu/upload.hpp"

#include <cstring>
#include <mutex>

namespace toaster::gpu::upload
{
	struct PendingUpload
	{
		enum class EType : uint8
		{
			eBuffer, eTexture
		};

		std::vector<uint8> data;
		uint64             destinationOffset{0u};
		uint64             handle{0u};
		TextureUploadDesc  textureUploadDesc{};
		EType              type{EType::eBuffer};
	};

	// Essentially a deferred deletion of the staging buffer but also tied to a command list
	struct SubmittedUpload
	{
		BufferHandle      staging{nullptr};
		CommandListHandle commandList{nullptr};
		uint64            timelineValue{0u};
	};

	struct UploadContextImpl
	{
		std::vector<PendingUpload>   pending;
		std::vector<SubmittedUpload> submitted;

		std::mutex uploadMutex;
	};

	static UploadContextImpl *g_impl{nullptr};

	constexpr uint64 stagingAlignment{16u}; // I think this should cover most things...

	auto collectCompletedUploads() -> void
	{
		const uint64 completedValue{getSemaphoreValue(frame::getTransferTimelineSemaphore())};
		for (auto it{g_impl->submitted.begin()}; it != g_impl->submitted.end();)
		{
			if (completedValue >= it->timelineValue)
			{
				destroyBuffer(it->staging);
				resetCommandList(it->commandList);
				freeCommandList(it->commandList); // Return the associated command list to the free pool
				it = g_impl->submitted.erase(it); // Why is C++ like ts
			}
			else
				++it;
		}
	}

	auto initUploadContext(const UploadContextDesc &) -> void
	{
		if (g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Upload context has already been initialised!");
			return;
		}

		g_impl = new UploadContextImpl{};
	}

	auto shutdownUploadContext() -> void
	{
		if (!g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Upload context has not been initialised!");
			return;
		}

		waitQueueIdle(EQueueType::eTransfer);
		collectCompletedUploads();
		for (const SubmittedUpload &upload: g_impl->submitted)
			destroyBuffer(upload.staging);

		delete g_impl;
		g_impl = nullptr;
	}

	auto flushUploads() -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};

		collectCompletedUploads();
		if (g_impl->pending.empty())
			return;

		uint64 stagingSize{0u};
		for (const PendingUpload &upload: g_impl->pending)
			stagingSize = TST_ALIGN(stagingSize, stagingAlignment) + upload.data.size();

		BufferDesc staging_desc{};
		staging_desc.size       = stagingSize;
		staging_desc.usage      = EBufferUsageFlagBits::eTransferSrc;
		staging_desc.memoryType = EMemoryType::eHostVisibleCoherent;
		BufferHandle staging{createBuffer(staging_desc)};

		CommandListHandle command_list{getOrCreateCommandList(EQueueType::eTransfer)};
		openCommandList(command_list);

		uint64 staging_offset{0u};
		for (const auto &upload: g_impl->pending)
		{
			staging_offset = TST_ALIGN(staging_offset, stagingAlignment);
			writeBufferData(staging, upload.data.data(), upload.data.size(), staging_offset);

			switch (upload.type)
			{
				case PendingUpload::EType::eBuffer:
				{
					copyBuffer(command_list, staging, static_cast<BufferHandle>(upload.handle), upload.data.size(), staging_offset, upload.destinationOffset);

					break;
				}
				case PendingUpload::EType::eTexture:
				{
					copyBufferToTexture(command_list, staging, static_cast<TextureHandle>(upload.handle), staging_offset, upload.textureUploadDesc.mipLevel,
										upload.textureUploadDesc.baseLayer, upload.textureUploadDesc.layerCount, upload.textureUploadDesc.extent);
					break;
				}
			}

			staging_offset += upload.data.size();
		}
		closeCommandList(command_list);

		const uint64 timeline_value{frame::acquireTransferTimelineCounterValue()};

		submit(EQueueType::eTransfer, command_list, {}, {{frame::getTransferTimelineSemaphore(), timeline_value}});
		g_impl->submitted.emplace_back(SubmittedUpload{staging, command_list, timeline_value});
		g_impl->pending.clear();
	}

	auto flushUploadsAndWait() -> void
	{
		flushUploads();
		waitSemaphores(frame::getTransferTimelineSemaphore(), frame::getTransferTimelineCounterValue());
		collectCompletedUploads();
	}

	auto uploadDataToBuffer(BufferHandle p_dst_buffer, const void *p_data, uint64 p_size, uint64 p_offset) -> uint64
	{
		TST_ASSERT_MSG(p_data != nullptr && p_size > 0u, "Upload data must actually exist");
		PendingUpload upload{};
		upload.type              = PendingUpload::EType::eBuffer;
		upload.handle            = static_cast<uint64>(p_dst_buffer);
		upload.destinationOffset = p_offset;
		upload.data.resize(p_size);
		std::memcpy(upload.data.data(), p_data, p_size);

		// Only lock on the critical sections
		std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};

		g_impl->pending.emplace_back(std::move(upload));
		return frame::getTransferTimelineCounterValue() + 1u;
	}

	auto uploadDataToTexture(TextureHandle p_dst_texture, const void *p_data, uint64 p_size, const TextureUploadDesc &p_desc) -> uint64
	{
		TST_ASSERT_MSG(p_data != nullptr && p_size > 0u, "Texture upload data must actually exist");
		TST_ASSERT_MSG(p_desc.layerCount > 0u, "Texture upload layer count must be non-zero");

		PendingUpload upload{};
		upload.type              = PendingUpload::EType::eTexture;
		upload.handle            = static_cast<uint64>(p_dst_texture);
		upload.textureUploadDesc = p_desc;
		upload.data.resize(p_size);
		std::memcpy(upload.data.data(), p_data, p_size);

		// Only lock on the critical sections
		std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};

		g_impl->pending.emplace_back(std::move(upload));
		return frame::getTransferTimelineCounterValue() + 1u;
	}

	auto cancelBufferUpload(BufferHandle p_buffer) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};

		for (auto it{g_impl->pending.begin()}; it != g_impl->pending.end();)
		{
			if (it->handle == static_cast<uint64>(p_buffer) && it->type == PendingUpload::EType::eBuffer)
				it = g_impl->pending.erase(it);
			else
				++it;
		}
	}

	auto cancelTextureUpload(TextureHandle p_texture) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};

		for (auto it{g_impl->pending.begin()}; it != g_impl->pending.end();)
		{
			if (it->handle == static_cast<uint64>(p_texture) && it->type == PendingUpload::EType::eTexture)
				it = g_impl->pending.erase(it);
			else
				++it;
		}
	}
}
