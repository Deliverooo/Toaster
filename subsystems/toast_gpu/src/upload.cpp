#include "toast_gpu/upload.hpp"

#include <cstring>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "toast_gpu/allocation.hpp"
#include "toast_lib/atomic.hpp"
#include "toast_lib/pool.hpp"

namespace toaster::gpu::upload
{
	// struct StagingAllocation
	// {
	// 	BufferHandle                   buffer{nullptr};
	// 	uint64                         bufferOffset{0u};
	// 	alloc::VirtualAllocationHandle virtualAllocation{nullptr};
	// 	void *                         mappedData{nullptr};
	// };

	struct StateTracker
	{
		UniquePtr<std::mutex>    ticketMutex{nullptr};
		std::vector<uint64>      timelineTickets;
		StateTrackerReadyFn      readyCb{nullptr};
		void *                   readyUserData{nullptr};
		TST_ALIGN_ATOMIC(uint32) pendingSubresources{0u};
	};

	static constexpr uint64 page_size{64u * 1024u * 1024u};

	struct StagingPage
	{
		BufferHandle              buffer{nullptr};
		alloc::VirtualBlockHandle block{nullptr};
		void *                    mappedStart{nullptr};
		uint64                    size{0u};
		uint32                    id{0u};

		TST_ALIGN_ATOMIC(uint32) activeAllocations{0u};
	};

	struct PageAllocation
	{
		BufferHandle                   buffer{nullptr};
		void *                         mapped{nullptr};
		alloc::VirtualAllocationHandle allocation{nullptr};
		uint64                         offset{0u};
		uint32                         pageId{0u};
	};

	struct UploadTask
	{
		enum class EType : uint8
		{
			eBuffer, eTexture
		};

		PageAllocation     stagingAllocation{};
		StateTrackerHandle stateTracker{nullptr};
		uint64             size{0u};
		uint64             dstOffset{0u};
		uint64             handle{0u};
		EType              type{EType::eBuffer};
	};

	struct LiveUploadBatch
	{
		std::vector<PageAllocation> allocations;
		uint64                      timelineValue{0u};
		// TODO: With dynamic paged allocation, I would like to have it so if an upload is too big, it will create a dedicated staging buffer and then free that here as well
	};

	struct UploadContextImpl
	{
		uint32                                  nextPageId{0u};
		uint32                                  activePageId{0u};
		std::unordered_map<uint32, StagingPage> stagingPages;

		Pool<StateTracker> stateTrackers;

		uint32                         maxAllocationCommandLists{3u};
		std::vector<CommandListHandle> commandLists;
		std::vector<uint64>            uploadTimeline;

		std::thread             uploadThread;
		std::atomic_bool        threadRunning{true};
		std::condition_variable cv;

		std::mutex pageMutex;
		std::mutex uploadMutex;
		std::mutex batchMutex;
		std::mutex activeStateTrackerMutex;

		std::queue<UploadTask>                 taskQueue;
		std::vector<LiveUploadBatch>           liveBatches;
		std::unordered_set<StateTrackerHandle> activeStateTrackers; // State trackers that are not finished
	};

	static UploadContextImpl *g_impl{nullptr};

	static auto createPage(uint64 p_size, uint32 p_id) -> StagingPage
	{
		StagingPage page{};
		page.size = p_size;
		page.id   = p_id;

		BufferDesc page_desc{};
		page_desc.size       = p_size;
		page_desc.memoryType = EMemoryType::eHostVisibleCoherent;
		page_desc.usage      = EBufferUsageFlagBits::eTransferSrc;
		page.buffer          = createBuffer(page_desc);
		page.mappedStart     = getBufferMappedData(page.buffer);

		page.block = alloc::createVirtualBlock(p_size);

		return page;
	}

	static auto destroyPage(StagingPage &p_page) -> void
	{
		alloc::destroyVirtualBlock(p_page.block);
		destroyBuffer(p_page.buffer);
	}

	static auto tryAllocate(StagingPage &p_page, uint64 p_size, uint64 p_alignment, PageAllocation &p_out_allocation) -> bool
	{
		p_out_allocation.allocation = alloc::virtualAllocate(p_page.block, p_size, p_alignment);
		if (p_out_allocation.allocation)
		{
			p_out_allocation.buffer = p_page.buffer;
			p_out_allocation.offset = alloc::getAllocationOffset(p_out_allocation.allocation);
			p_out_allocation.mapped = static_cast<uint8 *>(p_page.mappedStart) + p_out_allocation.offset;
			p_out_allocation.pageId = p_page.id;

			TST_SCOPED_ATOMIC(p_page.activeAllocations, active_allocations);
			++active_allocations;

			return true;
		}
		return false;
	}

	static auto freePageAllocation(StagingPage &p_page, const PageAllocation &p_allocation) -> void
	{
		alloc::virtualFree(p_allocation.allocation);
		TST_SCOPED_ATOMIC(p_page.activeAllocations, active_allocations);
		--active_allocations;
	}

	static auto insertPage() -> void
	{
		uint32 page_id{++g_impl->nextPageId};
		g_impl->stagingPages[page_id] = createPage(page_size, page_id);
		g_impl->activePageId          = page_id;
	}

	static auto allocateAcrossPages(uint64 p_size, PageAllocation &p_out_allocation) -> void
	{
		std::scoped_lock<std::mutex> page_lock{g_impl->pageMutex};

		if (tryAllocate(g_impl->stagingPages[g_impl->activePageId], p_size, 16u, p_out_allocation))
			return;

		for (auto &[page_id,page]: g_impl->stagingPages)
		{
			if (tryAllocate(page, p_size, 16u, p_out_allocation))
				return;
		}

		if (p_size > page_size)
		{
			uint32 page_id{++g_impl->nextPageId};
			g_impl->stagingPages[page_id] = createPage(p_size, page_id);
		}

		insertPage();
		if (!tryAllocate(g_impl->stagingPages[g_impl->activePageId], p_size, 16u, p_out_allocation))
		{
			TST_PERMA_ASSERT(false);
		}
	}

	static auto executeBatchSubmission(const std::vector<UploadTask> &p_tasks, uint32 p_cmd_index) -> void
	{
		uint64 current_value{getSemaphoreValue(frame::getTransferTimelineSemaphore())};

		CommandListHandle cmd{g_impl->commandLists[p_cmd_index]};

		if (current_value < g_impl->uploadTimeline[p_cmd_index])
			waitSemaphores(frame::getTransferTimelineSemaphore(), g_impl->uploadTimeline[p_cmd_index]);

		resetCommandList(cmd);
		openCommandList(cmd);

		LiveUploadBatch live_batch{};

		// std::vector<ImageMemoryBarrier>  image_barriers;
		// std::vector<BufferMemoryBarrier> buffer_barriers;

		for (const auto &task: p_tasks)
		{
			live_batch.allocations.push_back(task.stagingAllocation);

			switch (task.type)
			{
				case UploadTask::EType::eBuffer:
				{
					BufferHandle handle{static_cast<BufferHandle>(task.handle)};
					TST_ASSERT(handle.valid());

					copyBuffer(cmd, task.stagingAllocation.buffer, handle, task.size, task.stagingAllocation.offset, task.dstOffset);

					// auto &release_barrier{buffer_barriers.emplace_back()};

					break;
				}

				case UploadTask::EType::eTexture:
				{
					TextureHandle handle{static_cast<TextureHandle>(task.handle)};
					TST_ASSERT(handle.valid());

					copyBufferToTexture(cmd, task.stagingAllocation.buffer, handle, task.stagingAllocation.offset); // TODO: Information

					// auto &release_barrier{image_barriers.emplace_back()};

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
			TST_SCOPED_ATOMIC(tracker.pendingSubresources, pending_subresources);
			(void) --pending_subresources;
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

		g_impl->maxAllocationCommandLists = p_desc.maxAllocationCommandLists;

		insertPage(); // Default page

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
			p_data->pendingSubresources = 0u;

			p_data->readyCb       = nullptr;
			p_data->readyUserData = nullptr;
		});

		g_impl->uploadThread = std::thread(&transferMain);
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

		for (auto &[id, page]: g_impl->stagingPages)
			destroyPage(page);

		delete g_impl;
		g_impl = nullptr;
	}

	auto createStateTracker(uint32 p_expected_subresources) -> StateTrackerHandle
	{
		StateTracker state_tracker{};
		state_tracker.ticketMutex         = makeUnique<std::mutex>();
		state_tracker.pendingSubresources = p_expected_subresources;
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
		tracker.pendingSubresources = p_pending_subresources;
	}

	auto isStateTrackerReady(StateTrackerHandle p_state_tracker) -> bool
	{
		StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};

		std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

		TST_SCOPED_ATOMIC(tracker.pendingSubresources, pending_subresources);
		return !g_impl->activeStateTrackers.contains(p_state_tracker) && (pending_subresources.load() == 0u);
	}

	auto pollUploads() -> void
	{
		uint64 current_value{getSemaphoreValue(frame::getTransferTimelineSemaphore())};

		{
			std::scoped_lock<std::mutex> batch_lock{g_impl->batchMutex}; // Locks so multiple threads cannot call ts at once
			std::scoped_lock<std::mutex> page_lock{g_impl->pageMutex};

			for (auto it{g_impl->liveBatches.begin()}; it != g_impl->liveBatches.end();)
			{
				if (current_value >= it->timelineValue)
				{
					for (const auto &alloc: it->allocations)
					{
						auto page_it{g_impl->stagingPages.find(alloc.pageId)};
						if (page_it != g_impl->stagingPages.end())
						{
							freePageAllocation(page_it->second, alloc);
							TST_SCOPED_ATOMIC(page_it->second.activeAllocations, active_allocations);

							if (active_allocations.load() == 0u && page_it->first != g_impl->activePageId)
							{
								destroyPage(page_it->second);
								g_impl->stagingPages.erase(page_it);
							}
						}
					}

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

				TST_SCOPED_ATOMIC(tracker.pendingSubresources, pending_subresources);
				if (all_finished && pending_subresources.load() == 0u)
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
		UploadTask upload_task{};
		allocateAcrossPages(p_upload_desc.size, upload_task.stagingAllocation);

		std::memcpy(upload_task.stagingAllocation.mapped, p_upload_desc.data, p_upload_desc.size);

		{
			std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

			StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};

			TST_SCOPED_ATOMIC(tracker.pendingSubresources, pending_subresources);
			TST_ASSERT(pending_subresources.load() != 0u);

			g_impl->activeStateTrackers.insert(p_state_tracker);
		}

		upload_task.dstOffset    = p_upload_desc.dstOffset;
		upload_task.stateTracker = p_state_tracker;
		upload_task.size         = p_upload_desc.size;
		upload_task.handle       = static_cast<uint64>(p_upload_desc.dstBuffer);
		upload_task.type         = UploadTask::EType::eBuffer;

		{
			std::unique_lock<std::mutex> lock{g_impl->uploadMutex};
			g_impl->taskQueue.push(upload_task);
			g_impl->cv.notify_one();
		}
	}

	auto uploadDataToTexture(const TextureUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void
	{
		UploadTask upload_task{};
		allocateAcrossPages(p_upload_desc.size, upload_task.stagingAllocation);

		std::memcpy(upload_task.stagingAllocation.mapped, p_upload_desc.data, p_upload_desc.size);

		{
			std::scoped_lock<std::mutex> lock{g_impl->activeStateTrackerMutex};

			StateTracker &tracker{g_impl->stateTrackers[p_state_tracker]};

			TST_SCOPED_ATOMIC(tracker.pendingSubresources, pending_subresources);
			TST_ASSERT(pending_subresources.load() != 0u);

			g_impl->activeStateTrackers.insert(p_state_tracker);
		}

		upload_task.stateTracker = p_state_tracker;
		upload_task.size         = p_upload_desc.size;
		upload_task.handle       = static_cast<uint64>(p_upload_desc.dstTexture);
		upload_task.type         = UploadTask::EType::eTexture;

		{
			std::unique_lock<std::mutex> lock{g_impl->uploadMutex};
			g_impl->taskQueue.push(upload_task);
			g_impl->cv.notify_one();
		}
	}
}
