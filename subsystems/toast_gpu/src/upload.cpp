#include "toast_gpu/upload.hpp"

#include <cstring>
#include <mutex>
#include <queue>

#include "toast_gpu/allocation.hpp"

namespace toaster::gpu::upload
{
	struct StagingAllocation
	{
		BufferHandle                   buffer{nullptr};
		alloc::VirtualAllocationHandle virtualAllocation{nullptr};
		void *                         mappedData{nullptr};
	};

	struct UploadTask
	{
		enum class EType : uint8
		{
			eBuffer, eTexture
		};

		StagingAllocation    stagingAllocation{};
		RefPtr<StateTracker> stateTracker{nullptr};
		uint64               size{0u};
		uint64               handle{0u};
		EType                type{EType::eBuffer};
	};

	struct LiveUploadBatch
	{
		std::vector<alloc::VirtualAllocationHandle> allocationsToFree;
		uint64                                      timelineValue{0u};
		// TODO: With dynamic paged allocation, I would like to have it so if an upload is too big, it will create a dedicated staging buffer and then free that here as well
	};

	struct UploadContextImpl
	{
		BufferHandle              stagingBuffer{nullptr};
		alloc::VirtualBlockHandle stagingBlock{nullptr};

		void * mappedStart{nullptr};
		uint64 maxStagingSize{1024u * 1024u * 10u};
		uint64 lastAllocatedOffset{0u};

		uint32                         maxAllocationCommandLists{3u};
		std::vector<CommandListHandle> commandLists;
		std::vector<uint64>            uploadTimeline;

		std::thread             uploadThread;
		std::atomic_bool        threadRunning{true};
		std::condition_variable cv;

		std::mutex stagingMutex;
		std::mutex uploadMutex;
		std::mutex batchMutex;

		std::queue<UploadTask>       taskQueue;
		std::vector<LiveUploadBatch> liveBatches;
	};

	static UploadContextImpl *g_impl{nullptr};

	static auto executeBatchSubmission(const std::vector<UploadTask> &p_tasks, uint32 p_cmd_index) -> void
	{
		uint64 current_value{getSemaphoreValue(frame::getTransferTimelineSemaphore())};

		CommandListHandle cmd{g_impl->commandLists[p_cmd_index]};

		if (current_value < g_impl->uploadTimeline[p_cmd_index])
		{
			waitSemaphores(frame::getTransferTimelineSemaphore(), g_impl->uploadTimeline[p_cmd_index]);
			resetCommandList(cmd);
		}

		openCommandList(cmd);

		LiveUploadBatch live_batch{};

		for (const auto &task: p_tasks)
		{
			live_batch.allocationsToFree.push_back(task.stagingAllocation.virtualAllocation);

			switch (task.type)
			{
				case UploadTask::EType::eBuffer:
				{
					copyBuffer(cmd, task.stagingAllocation.buffer, static_cast<BufferHandle>(task.handle), task.size);
					break;
				}

				case UploadTask::EType::eTexture:
				{
					copyBufferToTexture(cmd, task.stagingAllocation.buffer, static_cast<TextureHandle>(task.handle)); // TODO: Information
					break;
				}
			}
		}

		closeCommandList(cmd);

		uint64 target_signal_value{frame::acquireTransferTimelineCounterValue()};
		g_impl->uploadTimeline[p_cmd_index] = target_signal_value;
		live_batch.timelineValue            = target_signal_value;

		submit(EQueueType::eTransfer, cmd, {}, SemaphoreSubmitInfo{frame::getTransferTimelineSemaphore(), target_signal_value});

		for (const auto &task: p_tasks)
		{
			{
				std::scoped_lock<std::mutex> ticket_lock{task.stateTracker->ticketMutex};
				task.stateTracker->timelineTickets.push_back(target_signal_value);
			}

			--task.stateTracker->pendingSubresources;
		}

		{
			std::scoped_lock<std::mutex> lock{g_impl->batchMutex};
			g_impl->liveBatches.push_back(live_batch);
		}
	}

	static auto transferMain() -> void
	{
		uint32 cmd_index{0u};

		while (g_impl->threadRunning)
		{
			std::vector<UploadTask> batch_tasks;

			{
				std::unique_lock<std::mutex> lock{g_impl->uploadMutex};
				g_impl->cv.wait(lock, +[]() -> bool { return !g_impl->taskQueue.empty() || !g_impl->threadRunning.load(); });
				if (!g_impl->taskQueue.empty() && !g_impl->threadRunning.load())
					break;

				while (!g_impl->taskQueue.empty())
				{
					batch_tasks.push_back(g_impl->taskQueue.front());
					g_impl->taskQueue.pop();
				}
			}

			if (!batch_tasks.empty())
			{
				executeBatchSubmission(batch_tasks, cmd_index);
				cmd_index = (cmd_index + 1u) % g_impl->maxAllocationCommandLists;
			}
		}
	}

	auto initUploadContext(const UploadContextDesc &p_desc) -> void
	{
		if (g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Upload context has already been initialised!");
			return;
		}

		g_impl = new UploadContextImpl{};

		g_impl->maxStagingSize            = p_desc.maxStagingSize;
		g_impl->maxAllocationCommandLists = p_desc.maxAllocationCommandLists;

		BufferDesc staging_buffer_desc{};
		staging_buffer_desc.size       = g_impl->maxStagingSize;
		staging_buffer_desc.usage      = EBufferUsageFlagBits::eTransferSrc;
		staging_buffer_desc.memoryType = EMemoryType::eHostVisibleCoherent;
		g_impl->stagingBuffer          = createBuffer(staging_buffer_desc);

		g_impl->mappedStart  = getBufferMappedData(g_impl->stagingBuffer);
		g_impl->stagingBlock = alloc::createVirtualBlock(g_impl->maxStagingSize);

		g_impl->commandLists.resize(g_impl->maxAllocationCommandLists);
		g_impl->uploadTimeline.resize(g_impl->maxAllocationCommandLists);
		for (uint32 i{0u}; i < g_impl->maxAllocationCommandLists; ++i)
		{
			g_impl->uploadTimeline[i] = 0u;
			g_impl->commandLists[i]   = getOrCreateCommandList(EQueueType::eTransfer);
		}

		g_impl->uploadThread = std::thread(&transferMain);
	}

	static auto allocateStagingMemory(uint64 p_size, uint64 p_alignment = 16u) -> StagingAllocation
	{
		std::scoped_lock<std::mutex> lock{g_impl->stagingMutex};

		const alloc::VirtualAllocationHandle virtual_allocation{alloc::virtualAllocate(g_impl->stagingBlock, p_size, p_alignment)};
		const uint64                         allocation_offset{alloc::getAllocationOffset(virtual_allocation)};

		StagingAllocation allocation{};
		allocation.buffer            = g_impl->stagingBuffer;
		allocation.virtualAllocation = virtual_allocation;
		allocation.mappedData        = static_cast<uint8 *>(g_impl->mappedStart) + allocation_offset;

		g_impl->lastAllocatedOffset = allocation_offset + p_size;

		return allocation;
	}

	auto shutdownUploadContext() -> void
	{
		if (!g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Upload context has not been initialised!");
			return;
		}

		alloc::destroyVirtualBlock(g_impl->stagingBlock);
		destroyBuffer(g_impl->stagingBuffer);

		delete g_impl;
		g_impl = nullptr;
	}

	auto pollUploads(const std::vector<RefPtr<StateTracker> > &p_active_state_trackers) -> void
	{
		uint64 current_value{getSemaphoreValue(frame::getTransferTimelineSemaphore())};

		{
			std::scoped_lock<std::mutex> lock{g_impl->batchMutex}; // Locks so multiple threads cannot call ts at once

			for (auto it{g_impl->liveBatches.begin()}; it != g_impl->liveBatches.end();)
			{
				if (current_value >= it->timelineValue)
				{
					for (auto virtual_alloc: it->allocationsToFree)
						alloc::virtualFree(virtual_alloc);

					it = g_impl->liveBatches.erase(it);
				}
				else
					++it;
			}
		}

		for (auto &tracker: p_active_state_trackers)
		{
			if (tracker->ready.load())
				continue;

			bool all_finished{true};
			{
				std::scoped_lock<std::mutex> lock{tracker->ticketMutex};
				if (tracker->timelineTickets.empty()) // If the tracker has no timeline tickets, it indicates that worker threads have not yet begun an upload on it
					all_finished = false;

				for (const uint64 ticket: tracker->timelineTickets)
				{
					if (current_value < ticket)
					{
						all_finished = false;
						break;
					}
				}
			}

			if (all_finished && tracker->pendingSubresources.load() == 0u)
				tracker->ready.store(true);
		}
	}

	auto uploadDataToBuffer(const BufferUploadDesc &p_upload_desc, RefPtr<StateTracker> &p_state_tracker) -> void
	{
		StagingAllocation staging_allocation{allocateStagingMemory(p_upload_desc.size)};

		std::memcpy(staging_allocation.mappedData, p_upload_desc.data, p_upload_desc.size);

		UploadTask upload_task{};
		upload_task.stagingAllocation = staging_allocation;
		upload_task.stateTracker      = p_state_tracker;
		upload_task.size              = p_upload_desc.size;
		upload_task.handle            = p_upload_desc.dstBuffer;
		upload_task.type              = UploadTask::EType::eBuffer;

		{
			std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
			g_impl->taskQueue.push(upload_task);
		}
		g_impl->cv.notify_one();
	}

	auto uploadDataToTexture(const TextureUploadDesc &p_upload_desc, RefPtr<StateTracker> &p_state_tracker) -> void
	{
		StagingAllocation staging_allocation{allocateStagingMemory(p_upload_desc.size)};

		std::memcpy(staging_allocation.mappedData, p_upload_desc.data, p_upload_desc.size);

		UploadTask upload_task{};
		upload_task.stagingAllocation = staging_allocation;
		upload_task.stateTracker      = p_state_tracker;
		upload_task.size              = p_upload_desc.size;
		upload_task.handle            = p_upload_desc.dstTexture;
		upload_task.type              = UploadTask::EType::eTexture;

		{
			std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
			g_impl->taskQueue.push(upload_task);
		}
		g_impl->cv.notify_one();
	}

	// auto flushUploads() -> void
	// {
	// std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
	//
	// collectCompletedUploads();
	// if (g_impl->pending.empty())
	// 	return;
	//
	// uint64 stagingSize{0u};
	// for (const PendingUpload &upload: g_impl->pending)
	// 	stagingSize = TST_ALIGN(stagingSize, stagingAlignment) + upload.data.size();
	//
	// BufferDesc staging_desc{};
	// staging_desc.size       = stagingSize;
	// staging_desc.usage      = EBufferUsageFlagBits::eTransferSrc;
	// staging_desc.memoryType = EMemoryType::eHostVisibleCoherent;
	// BufferHandle staging{createBuffer(staging_desc)};
	//
	// CommandListHandle command_list{getOrCreateCommandList(EQueueType::eTransfer)};
	// openCommandList(command_list);
	//
	// uint64 staging_offset{0u};
	// for (const auto &upload: g_impl->pending)
	// {
	// 	staging_offset = TST_ALIGN(staging_offset, stagingAlignment);
	// 	writeBufferData(staging, upload.data.data(), upload.data.size(), staging_offset);
	//
	// 	switch (upload.type)
	// 	{
	// 		case PendingUpload::EType::eBuffer:
	// 		{
	// 			copyBuffer(command_list, staging, static_cast<BufferHandle>(upload.handle), upload.data.size(), staging_offset, upload.destinationOffset);
	//
	// 			break;
	// 		}
	// 		case PendingUpload::EType::eTexture:
	// 		{
	// 			copyBufferToTexture(command_list, staging, static_cast<TextureHandle>(upload.handle), staging_offset, upload.textureUploadDesc.mipLevel,
	// 								upload.textureUploadDesc.baseLayer, upload.textureUploadDesc.layerCount, upload.textureUploadDesc.extent);
	// 			break;
	// 		}
	// 	}
	//
	// 	staging_offset += upload.data.size();
	// }
	// closeCommandList(command_list);
	//
	// const uint64 timeline_value{frame::acquireTransferTimelineCounterValue()};
	//
	// submit(EQueueType::eTransfer, command_list, {}, {{frame::getTransferTimelineSemaphore(), timeline_value}});
	// g_impl->submitted.emplace_back(SubmittedUpload{staging, command_list, timeline_value});
	// g_impl->pending.clear();
	// }

	// auto flushUploadsAndWait() -> void
	// {
	// 	flushUploads();
	// 	waitSemaphores(frame::getTransferTimelineSemaphore(), frame::getTransferTimelineCounterValue());
	// 	collectCompletedUploads();
	// }
	//
	// auto uploadDataToBuffer(BufferHandle p_dst_buffer, const void *p_data, uint64 p_size, uint64 p_offset) -> uint64
	// {
	// 	TST_ASSERT_MSG(p_data != nullptr && p_size > 0u, "Upload data must actually exist");
	// 	PendingUpload upload{};
	// 	upload.type              = PendingUpload::EType::eBuffer;
	// 	upload.handle            = static_cast<uint64>(p_dst_buffer);
	// 	upload.destinationOffset = p_offset;
	// 	upload.data.resize(p_size);
	// 	std::memcpy(upload.data.data(), p_data, p_size);
	//
	// 	// Only lock on the critical sections
	// 	std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
	//
	// 	g_impl->pending.emplace_back(std::move(upload));
	// 	return frame::getTransferTimelineCounterValue() + 1u;
	// }
	//
	// auto uploadDataToTexture(TextureHandle p_dst_texture, const void *p_data, uint64 p_size, const TextureUploadDesc &p_desc) -> uint64
	// {
	// 	TST_ASSERT_MSG(p_data != nullptr && p_size > 0u, "Texture upload data must actually exist");
	// 	TST_ASSERT_MSG(p_desc.layerCount > 0u, "Texture upload layer count must be non-zero");
	//
	// 	PendingUpload upload{};
	// 	upload.type              = PendingUpload::EType::eTexture;
	// 	upload.handle            = static_cast<uint64>(p_dst_texture);
	// 	upload.textureUploadDesc = p_desc;
	// 	upload.data.resize(p_size);
	// 	std::memcpy(upload.data.data(), p_data, p_size);
	//
	// 	// Only lock on the critical sections
	// 	std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
	//
	// 	g_impl->pending.emplace_back(std::move(upload));
	// 	return frame::getTransferTimelineCounterValue() + 1u;
	// }
	//
	// auto cancelBufferUpload(BufferHandle p_buffer) -> void
	// {
	// 	std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
	//
	// 	for (auto it{g_impl->pending.begin()}; it != g_impl->pending.end();)
	// 	{
	// 		if (it->handle == static_cast<uint64>(p_buffer) && it->type == PendingUpload::EType::eBuffer)
	// 			it = g_impl->pending.erase(it);
	// 		else
	// 			++it;
	// 	}
	// }
	//
	// auto cancelTextureUpload(TextureHandle p_texture) -> void
	// {
	// 	std::scoped_lock<std::mutex> lock{g_impl->uploadMutex};
	//
	// 	for (auto it{g_impl->pending.begin()}; it != g_impl->pending.end();)
	// 	{
	// 		if (it->handle == static_cast<uint64>(p_texture) && it->type == PendingUpload::EType::eTexture)
	// 			it = g_impl->pending.erase(it);
	// 		else
	// 			++it;
	// 	}
	// }
}
