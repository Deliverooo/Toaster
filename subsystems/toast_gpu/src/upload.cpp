#include "toast_gpu/upload.hpp"

#include <cstring>
#include <mutex>
#include <queue>
#include <unordered_set>

#include "toast_gpu/allocation.hpp"
#include "toast_lib/pool.hpp"

namespace toaster::gpu::upload
{
	struct StagingAllocation
	{
		BufferHandle                   buffer{nullptr};
		uint64                         bufferOffset{0u};
		alloc::VirtualAllocationHandle virtualAllocation{nullptr};
		void *                         mappedData{nullptr};
	};

	struct StateTracker
	{
		UniquePtr<std::mutex>           ticketMutex{nullptr};
		std::vector<uint64>             timelineTickets;
		UniquePtr<std::atomic_uint32_t> pendingSubresources{nullptr};

		StateTrackerReadyFn readyCb{nullptr};
		void *              readyUserData{nullptr};
	};

	struct UploadTask
	{
		enum class EType : uint8
		{
			eBuffer, eTexture
		};

		StagingAllocation  stagingAllocation{};
		StateTrackerHandle stateTracker{nullptr};
		uint64             size{0u};
		uint64             dstOffset{0u};

		uint64 handle{0u};

		EType type{EType::eBuffer};

		std::array<char, 4u> magic{'O', 'R', 'B', 'O'};
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
		uint64 maxStagingSize{1024u * 1024u * 250u};
		uint64 lastAllocatedOffset{0u};

		Pool<StateTracker> stateTrackers;

		uint32                         maxAllocationCommandLists{3u};
		std::vector<CommandListHandle> commandLists;
		std::vector<uint64>            uploadTimeline;

		std::thread             uploadThread;
		std::atomic_bool        threadRunning{true};
		std::condition_variable cv;

		std::mutex stagingMutex;
		std::mutex uploadMutex;
		std::mutex batchMutex;
		std::mutex activeStateTrackerMutex;

		std::queue<UploadTask>                 taskQueue;
		std::vector<LiveUploadBatch>           liveBatches;
		std::unordered_set<StateTrackerHandle> activeStateTrackers; // State trackers that are not finished
	};

	static UploadContextImpl *g_impl{nullptr};

	static auto executeBatchSubmission(const std::vector<UploadTask> &p_tasks, uint32 p_cmd_index) -> void
	{
		uint64 current_value{getSemaphoreValue(frame::getTransferTimelineSemaphore())};

		CommandListHandle cmd{g_impl->commandLists[p_cmd_index]};

		if (current_value < g_impl->uploadTimeline[p_cmd_index])
		{
			waitSemaphores(frame::getTransferTimelineSemaphore(), g_impl->uploadTimeline[p_cmd_index]);
		}

		// if (g_impl->uploadTimeline[p_cmd_ndex] != 0u)
		resetCommandList(cmd);

		openCommandList(cmd);

		LiveUploadBatch live_batch{};

		for (const auto &task: p_tasks)
		{
			live_batch.allocationsToFree.push_back(task.stagingAllocation.virtualAllocation);

			switch (task.type)
			{
				case UploadTask::EType::eBuffer:
				{
					BufferHandle handle{static_cast<BufferHandle>(task.handle)};
					TST_ASSERT(handle.valid());
					copyBuffer(cmd, task.stagingAllocation.buffer, handle, task.size, task.stagingAllocation.bufferOffset, task.dstOffset);
					break;
				}

				case UploadTask::EType::eTexture:
				{
					TextureHandle handle{static_cast<TextureHandle>(task.handle)};
					TST_ASSERT(handle.valid());
					copyBufferToTexture(cmd, task.stagingAllocation.buffer, handle, task.stagingAllocation.bufferOffset); // TODO: Information
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
			StateTracker &tracker{g_impl->stateTrackers[task.stateTracker]};
			{
				std::scoped_lock<std::mutex> ticket_lock{*tracker.ticketMutex};
				tracker.timelineTickets.push_back(target_signal_value);
			}

			--(*tracker.pendingSubresources);
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
				if (g_impl->taskQueue.empty() && !g_impl->threadRunning.load())
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

		g_impl->stateTrackers.setDestructorFn(+[](StateTracker *p_data, void *) -> void
		{
			p_data->timelineTickets.clear();
			p_data->pendingSubresources->store(0u);

			p_data->readyCb       = nullptr;
			p_data->readyUserData = nullptr;
		});

		g_impl->uploadThread = std::thread(&transferMain);
	}

	static auto allocateStagingMemory(uint64 p_size, uint64 p_alignment = 16u) -> StagingAllocation
	{
		std::scoped_lock<std::mutex> lock{g_impl->stagingMutex};

		const alloc::VirtualAllocationHandle virtual_allocation{alloc::virtualAllocate(g_impl->stagingBlock, p_size, p_alignment)};
		const uint64                         allocation_offset{alloc::getAllocationOffset(virtual_allocation)};

		StagingAllocation allocation{};
		allocation.buffer            = g_impl->stagingBuffer;
		allocation.bufferOffset      = allocation_offset;
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

		g_impl->threadRunning.store(false);
		g_impl->cv.notify_one();
		if (g_impl->uploadThread.joinable())
			g_impl->uploadThread.join();

		g_impl->stateTrackers.clear();

		alloc::destroyVirtualBlock(g_impl->stagingBlock);
		destroyBuffer(g_impl->stagingBuffer);

		delete g_impl;
		g_impl = nullptr;
	}

	auto createStateTracker(uint32 p_expected_subresources) -> StateTrackerHandle
	{
		StateTracker state_tracker{};
		state_tracker.ticketMutex         = makeUnique<std::mutex>();
		state_tracker.pendingSubresources = makeUnique<std::atomic_uint32_t>(p_expected_subresources);
		return g_impl->stateTrackers.emplace(std::move(state_tracker));
	}

	auto registerStateTrackerReadyCallback(StateTrackerHandle p_state_tracker, StateTrackerReadyFn p_ready_callback, void *p_callback_user_data) -> void
	{
		TST_ASSERT_MSG(!g_impl->activeStateTrackers.contains(p_state_tracker), "State tracker is in use");

		StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};
		tracker.readyCb       = p_ready_callback;
		tracker.readyUserData = p_callback_user_data;
	}

	auto destroyStateTracker(StateTrackerHandle p_state_tracker) -> void
	{
		TST_ASSERT_MSG(!g_impl->activeStateTrackers.contains(p_state_tracker), "State tracker is in use");
		g_impl->stateTrackers.destroy(p_state_tracker);
	}

	auto resetStateTracker(StateTrackerHandle p_state_tracker, uint32 p_pending_subresources) -> void
	{
		TST_ASSERT_MSG(!g_impl->activeStateTrackers.contains(p_state_tracker), "State tracker is in use");

		StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};

		tracker.timelineTickets.clear();
		tracker.pendingSubresources->store(p_pending_subresources);
	}

	auto isStateTrackerReady(StateTrackerHandle p_state_tracker) -> bool
	{
		StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};

		std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

		const bool ready{!g_impl->activeStateTrackers.contains(p_state_tracker) && (tracker.pendingSubresources->load() == 0u)};

		return ready;
	}

	auto pollUploads() -> void
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

		{
			std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

			for (auto it{g_impl->activeStateTrackers.begin()}; it != g_impl->activeStateTrackers.end();)
			{
				StateTracker &tracker{g_impl->stateTrackers[*it]};

				bool all_finished{true};
				{
					std::scoped_lock<std::mutex> ticket_lock{*tracker.ticketMutex};
					if (tracker.timelineTickets.empty()) // If the tracker has no timeline tickets, it indicates that worker threads have not yet begun an upload on it
						all_finished = false;

					for (const uint64 ticket: tracker.timelineTickets)
					{
						if (current_value < ticket)
						{
							all_finished = false;
							break;
						}
					}
				}

				if (all_finished && tracker.pendingSubresources->load() == 0u)
				{
					if (tracker.readyCb)
						tracker.readyCb(tracker.readyUserData);

					it = g_impl->activeStateTrackers.erase(it);
				}
				else
					++it;
			}
		}
	}

	auto uploadDataToBuffer(const BufferUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void
	{
		StagingAllocation staging_allocation{allocateStagingMemory(p_upload_desc.size)};

		std::memcpy(staging_allocation.mappedData, p_upload_desc.data, p_upload_desc.size);

		UploadTask upload_task{};

		upload_task.stagingAllocation = staging_allocation;

		{
			std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

			StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};
			TST_ASSERT(tracker.pendingSubresources->load() != 0u);
			g_impl->activeStateTrackers.insert(p_state_tracker);
		}

		upload_task.dstOffset    = p_upload_desc.dstOffset;
		upload_task.stateTracker = p_state_tracker;
		upload_task.size         = p_upload_desc.size;
		upload_task.handle       = static_cast<uint64>(p_upload_desc.dstBuffer);
		upload_task.type         = UploadTask::EType::eBuffer;

		upload_task.magic = {'P', 'E', 'E', 'B'};

		{
			std::unique_lock<std::mutex> lock{g_impl->uploadMutex};
			g_impl->taskQueue.push(upload_task);
			g_impl->cv.notify_one();
		}
	}

	auto uploadDataToTexture(const TextureUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void
	{
		StagingAllocation staging_allocation{allocateStagingMemory(p_upload_desc.size)};

		std::memcpy(staging_allocation.mappedData, p_upload_desc.data, p_upload_desc.size);

		UploadTask upload_task{};
		upload_task.stagingAllocation = staging_allocation;

		{
			std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

			StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};
			TST_ASSERT(tracker.pendingSubresources->load() != 0u);
			g_impl->activeStateTrackers.insert(p_state_tracker);
		}

		upload_task.stateTracker = p_state_tracker;
		upload_task.size         = p_upload_desc.size;
		upload_task.handle       = static_cast<uint64>(p_upload_desc.dstTexture);
		upload_task.type         = UploadTask::EType::eTexture;

		upload_task.magic = {'P', 'E', 'E', 'B'};

		{
			std::unique_lock<std::mutex> lock{g_impl->uploadMutex};
			g_impl->taskQueue.push(upload_task);
			g_impl->cv.notify_one();
		}
	}
}
