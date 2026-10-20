#include "toast_gpu/upload.hpp"

#include <algorithm>
#include <queue>
#include <threads.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "toast_gpu/allocation.hpp"
#include "toast_lib/atomic.hpp"
#include "toast_lib/pool.hpp"

namespace toaster::gpu
{
	struct StateTracker
	{
		mtx_t                    ticketMutex;
		std::vector<uint64>      timelineTickets;
		StateTrackerReadyFn      readyCb{nullptr};
		TST_ALIGN_ATOMIC(uint32) pendingSubresources{0u};
	};

	static constexpr uint64 page_size{72u * 1024u * 1024u};

	struct StagingPage
	{
		BufferHandle       buffer{nullptr};
		VirtualBlockHandle block{nullptr};
		void *             mappedStart{nullptr};
		uint64             size{0u};
		uint32             id{0u};

		TST_ALIGN_ATOMIC(uint32) activeAllocations{0u};
	};

	struct UploadPageAllocation
	{
		BufferHandle            buffer{nullptr};
		void *                  mapped{nullptr};
		VirtualAllocationHandle allocation{nullptr};
		uint64                  offset{0u};
		uint32                  pageId{0u};
	};

	struct UploadTask
	{
		enum class EType : uint8
		{
			eBuffer, eTexture
		};

		UploadPageAllocation stagingAllocation{};
		StateTrackerHandle   stateTracker{nullptr};
		math::UInt3          extent{0u, 0u, 0u};
		math::Int3           offset{0, 0, 0};
		uint64               size{0u};
		uint64               dstOffset{0u};
		uint64               handle{0u};
		EType                type{EType::eBuffer};
	};

	struct LiveUploadBatch
	{
		std::vector<UploadPageAllocation> allocations;
		uint64                            timelineValue{0u};
		// TODO: With dynamic paged allocation, I would like to have it so if an upload is too big, it will create a dedicated staging buffer and then free that here as well
	};

	struct UploadContext
	{
		GPUContext *       gpuCtx{nullptr};
		AllocationContext *allocCtx{nullptr};

		SemaphoreHandle     transferTimelineSemaphore{nullptr};
		std::atomic<uint64> transferTimelineCounter{0u};

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

		std::queue<UploadTask>       taskQueue;
		std::vector<LiveUploadBatch> liveBatches;
	};

	static auto createPage(UploadContext *p_ctx, uint64 p_size, uint32 p_id) -> StagingPage
	{
		StagingPage page{};
		page.size = p_size;
		page.id   = p_id;

		BufferDesc page_desc{};
		page_desc.size       = p_size;
		page_desc.memoryType = EMemoryType::eHostVisibleCoherent;
		page_desc.usage      = EBufferUsageFlagBits::eTransferSrc;
		page.buffer          = buffer_create(p_ctx->gpuCtx, page_desc);
		page.mappedStart     = buffer_get_mapped_data(p_ctx->gpuCtx, page.buffer);

		page.block = virtual_block_create(p_ctx->allocCtx, p_size);

		return page;
	}

	static auto destroyPage(UploadContext *p_ctx, StagingPage &p_page) -> void
	{
		virtual_block_destroy(p_ctx->allocCtx, p_page.block);
		buffer_destroy(p_ctx->gpuCtx, p_page.buffer);
	}

	static auto tryAllocate(UploadContext *p_ctx, StagingPage &p_page, uint64 p_size, uint64 p_alignment, UploadPageAllocation &p_out_allocation) -> bool
	{
		p_out_allocation.allocation = virtual_block_allocate(p_ctx->allocCtx, p_page.block, p_size, p_alignment);
		if (p_out_allocation.allocation)
		{
			p_out_allocation.buffer = p_page.buffer;
			p_out_allocation.offset = virtual_allocation_get_offset(p_ctx->allocCtx, p_out_allocation.allocation);
			p_out_allocation.mapped = static_cast<uint8 *>(p_page.mappedStart) + p_out_allocation.offset;
			p_out_allocation.pageId = p_page.id;

			TST_SCOPED_ATOMIC(p_page.activeAllocations, active_allocations);
			++active_allocations;

			return true;
		}
		return false;
	}

	static auto freeUploadPageAllocation(UploadContext *p_ctx, StagingPage &p_page, const UploadPageAllocation &p_allocation) -> void
	{
		virtual_block_free(p_ctx->allocCtx, p_allocation.allocation);
		TST_SCOPED_ATOMIC(p_page.activeAllocations, active_allocations);
		--active_allocations;
	}

	static auto insertPage(UploadContext *p_ctx) -> void
	{
		uint32 page_id{++p_ctx->nextPageId};
		p_ctx->stagingPages[page_id] = createPage(p_ctx, page_size, page_id);
		p_ctx->activePageId          = page_id;
	}

	static auto allocateAcrossPages(UploadContext *p_ctx, uint64 p_size, UploadPageAllocation &p_out_allocation) -> void
	{
		std::scoped_lock<std::mutex> page_lock{p_ctx->pageMutex};

		if (tryAllocate(p_ctx, p_ctx->stagingPages[p_ctx->activePageId], p_size, 16u, p_out_allocation))
			return;

		for (auto &[page_id,page]: p_ctx->stagingPages)
		{
			if (tryAllocate(p_ctx, page, p_size, 16u, p_out_allocation))
				return;
		}

		if (p_size > page_size)
		{
			uint32 page_id{++p_ctx->nextPageId};
			p_ctx->stagingPages[page_id] = createPage(p_ctx, p_size, page_id);

			if (!tryAllocate(p_ctx, p_ctx->stagingPages[page_id], p_size, 16u, p_out_allocation))
				TST_PERMA_ASSERT(false);
			return;
		}

		insertPage(p_ctx);
		if (!tryAllocate(p_ctx, p_ctx->stagingPages[p_ctx->activePageId], p_size, 16u, p_out_allocation))
		{
			TST_PERMA_ASSERT(false);
		}
	}

	static auto executeBatchSubmission(UploadContext *p_ctx, const std::vector<UploadTask> &p_tasks, uint32 p_cmd_index) -> void
	{
		uint64 current_value{semaphore_get_value(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore)};

		CommandListHandle cmd{p_ctx->commandLists[p_cmd_index]};

		if (current_value < p_ctx->uploadTimeline[p_cmd_index])
			semaphore_wait_n(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore, p_ctx->uploadTimeline[p_cmd_index]);

		cmd_list_reset(p_ctx->gpuCtx, cmd);
		cmd_list_open(p_ctx->gpuCtx, cmd);

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

					cmd_copy_buffer(p_ctx->gpuCtx, cmd, task.stagingAllocation.buffer, handle, task.size, task.stagingAllocation.offset, task.dstOffset);

					// auto &release_barrier{buffer_barriers.emplace_back()};

					break;
				}

				case UploadTask::EType::eTexture:
				{
					TextureHandle handle{static_cast<TextureHandle>(task.handle)};
					TST_ASSERT(handle.valid());

					cmd_copy_buffer_to_texture(p_ctx->gpuCtx, cmd, task.stagingAllocation.buffer, handle, task.stagingAllocation.offset, 0u, 0u, 1u, task.extent,
											   task.offset);

					// auto &release_barrier{image_barriers.emplace_back()};

					break;
				}
			}
		}

		cmd_list_close(p_ctx->gpuCtx, cmd);

		uint64 target_signal_value{_acquire_transfer_timeline_counter_value(p_ctx)};
		p_ctx->uploadTimeline[p_cmd_index] = target_signal_value;
		live_batch.timelineValue           = target_signal_value;

		queue_submit(p_ctx->gpuCtx, EQueueType::eTransfer, cmd, {}, SemaphoreSubmitInfo{p_ctx->transferTimelineSemaphore, target_signal_value});

		for (const auto &task: p_tasks)
		{
			StateTracker &tracker{p_ctx->stateTrackers[task.stateTracker]};

			mtx_lock(&tracker.ticketMutex);
			tracker.timelineTickets.push_back(target_signal_value);
			mtx_unlock(&tracker.ticketMutex);

			TST_SCOPED_ATOMIC(tracker.pendingSubresources, pending_subresources);
			(void) --pending_subresources;
		}

		{
			std::scoped_lock<std::mutex> lock{p_ctx->batchMutex};
			p_ctx->liveBatches.push_back(live_batch);
		}
	}

	static auto transferMain(UploadContext *p_ctx) -> void
	{
		uint32 cmd_index{0u};

		while (p_ctx->threadRunning)
		{
			std::vector<UploadTask> batch_tasks;

			{
				std::unique_lock<std::mutex> lock{p_ctx->uploadMutex};
				p_ctx->cv.wait(lock, [p_ctx]() -> bool { return !p_ctx->taskQueue.empty() || !p_ctx->threadRunning.load(); });
				if (p_ctx->taskQueue.empty() && !p_ctx->threadRunning.load())
					break;

				while (!p_ctx->taskQueue.empty())
				{
					batch_tasks.push_back(p_ctx->taskQueue.front());
					p_ctx->taskQueue.pop();
				}
			}

			if (!batch_tasks.empty())
			{
				executeBatchSubmission(p_ctx, batch_tasks, cmd_index);
				cmd_index = (cmd_index + 1u) % p_ctx->maxAllocationCommandLists;
			}
		}
	}

	auto upload_ctx_create(GPUContext *p_gpu_ctx, AllocationContext *p_alloc_ctx, const UploadContextDesc &p_desc) -> UploadContext *
	{
		UploadContext *ctx{new UploadContext{}};

		ctx->gpuCtx   = p_gpu_ctx;
		ctx->allocCtx = p_alloc_ctx;

		ctx->transferTimelineSemaphore = semaphore_create(p_gpu_ctx);

		ctx->maxAllocationCommandLists = p_desc.maxAllocationCommandLists;

		insertPage(ctx); // Default page

		ctx->commandLists.resize(ctx->maxAllocationCommandLists);
		ctx->uploadTimeline.resize(ctx->maxAllocationCommandLists);
		for (uint32 i{0u}; i < ctx->maxAllocationCommandLists; ++i)
		{
			ctx->uploadTimeline[i] = 0u;
			ctx->commandLists[i]   = cmd_list_get_or_create(p_gpu_ctx, EQueueType::eTransfer);
		}

		ctx->stateTrackers.setDestructorFn(+[](StateTracker *p_data, void *) -> void
		{
			mtx_destroy(&p_data->ticketMutex);
			p_data->timelineTickets.clear();
			p_data->pendingSubresources = 0u;
			p_data->readyCb             = nullptr;
		});

		ctx->uploadThread = std::thread(&transferMain, ctx);

		return ctx;
	}

	auto upload_ctx_destroy(UploadContext *p_ctx) -> void
	{
		p_ctx->threadRunning.store(false);
		p_ctx->cv.notify_one();
		if (p_ctx->uploadThread.joinable())
			p_ctx->uploadThread.join();

		p_ctx->stateTrackers.clear();

		for (auto &[id, page]: p_ctx->stagingPages)
			destroyPage(p_ctx, page);

		semaphore_destroy(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore);

		delete p_ctx;
	}

	auto state_tracker_register(UploadContext *p_ctx, uint32 p_expected_subresources) -> StateTrackerHandle
	{
		StateTracker state_tracker{};
		mtx_init(&state_tracker.ticketMutex, mtx_plain);
		state_tracker.pendingSubresources = p_expected_subresources;
		return p_ctx->stateTrackers.emplace(std::move(state_tracker));
	}

	auto state_tracker_register_ready_callback(UploadContext *p_ctx, StateTrackerHandle p_state_tracker, StateTrackerReadyFn p_ready_callback) -> void
	{
		StateTracker *tracker{p_ctx->stateTrackers.tryGet(p_state_tracker)};
		TST_ASSERT_MSG(tracker, "State tracker does not exist or is already completed");
		tracker->readyCb = p_ready_callback;
	}

	auto state_tracker_is_ready(UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> bool
	{
		if (p_ctx->stateTrackers.isValid(p_state_tracker)) // If the state tracker is valid, it means that it has not completed all of it's operations
			return false;
		return true;
	}

	auto state_tracker_wait_for(UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> void
	{
		StateTracker *tracker{p_ctx->stateTrackers.tryGet(p_state_tracker)};
		TST_ASSERT_MSG(tracker, "State tracker does not exist or is already completed");

		while (true)
		{
			uint64 current_value{semaphore_get_value(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore)};

			bool all_finished{true};

			{
				mtx_lock(&tracker->ticketMutex);
				// If the tracker has no timeline tickets, it indicates that worker threads have not yet begun an upload on it
				if (tracker->timelineTickets.empty())
					all_finished = false;

				for (const uint64 ticket: tracker->timelineTickets)
				{
					if (current_value < ticket)
					{
						all_finished = false;
						break;
					}
				}
				mtx_unlock(&tracker->ticketMutex);
			}
			TST_SCOPED_ATOMIC(tracker->pendingSubresources, pending_subresources);
			if (all_finished && pending_subresources.load() == 0u)
			{
				if (tracker->readyCb)
					tracker->readyCb();

				p_ctx->stateTrackers.destroy(p_state_tracker);

				break;
			}
		}
	}

	auto poll_uploads(UploadContext *p_ctx) -> void
	{
		uint64 current_value{semaphore_get_value(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore)};

		{
			std::scoped_lock<std::mutex> batch_lock{p_ctx->batchMutex}; // Locks so multiple threads cannot call ts at once
			std::scoped_lock<std::mutex> page_lock{p_ctx->pageMutex};

			for (auto it{p_ctx->liveBatches.begin()}; it != p_ctx->liveBatches.end();)
			{
				if (current_value >= it->timelineValue)
				{
					for (const auto &alloc: it->allocations)
					{
						auto page_it{p_ctx->stagingPages.find(alloc.pageId)};
						if (page_it != p_ctx->stagingPages.end())
						{
							freeUploadPageAllocation(p_ctx, page_it->second, alloc);
							TST_SCOPED_ATOMIC(page_it->second.activeAllocations, active_allocations);

							if (active_allocations.load() == 0u && page_it->first != p_ctx->activePageId)
							{
								destroyPage(p_ctx, page_it->second);
								p_ctx->stagingPages.erase(page_it);
							}
						}
					}

					it = p_ctx->liveBatches.erase(it);
				}
				else
					++it;
			}
		}

		std::vector<uint32> indices_to_remove;
		p_ctx->stateTrackers.forEachAlive([&indices_to_remove, current_value](StateTracker &p_data, uint32 p_index) -> void
		{
			bool all_finished{true};

			{
				mtx_lock(&p_data.ticketMutex);
				// If the tracker has no timeline tickets, it indicates that worker threads have not yet begun an upload on it
				if (p_data.timelineTickets.empty())
					all_finished = false;

				for (const uint64 ticket: p_data.timelineTickets)
				{
					if (current_value < ticket)
					{
						all_finished = false;
						break;
					}
				}
				mtx_unlock(&p_data.ticketMutex);
			}

			TST_SCOPED_ATOMIC(p_data.pendingSubresources, pending_subresources);
			if (all_finished && pending_subresources.load() == 0u)
			{
				if (p_data.readyCb)
					p_data.readyCb();

				indices_to_remove.push_back(p_index); // The state tracker is ready, so it can be destroyed
			}
		});

		for (const auto index: indices_to_remove)
			p_ctx->stateTrackers.removeAt(index);
	}

	auto upload_data_to_buffer(UploadContext *p_ctx, const BufferUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void
	{
		TST_ASSERT(p_ctx->stateTrackers.isValid(p_state_tracker));

		UploadTask upload_task{};
		allocateAcrossPages(p_ctx, p_upload_desc.size, upload_task.stagingAllocation);

		std::memcpy(upload_task.stagingAllocation.mapped, p_upload_desc.data, p_upload_desc.size);

		upload_task.dstOffset    = p_upload_desc.dstOffset;
		upload_task.stateTracker = p_state_tracker;
		upload_task.size         = p_upload_desc.size;
		upload_task.handle       = static_cast<uint64>(p_upload_desc.dstBuffer);
		upload_task.type         = UploadTask::EType::eBuffer;

		{
			std::unique_lock<std::mutex> lock{p_ctx->uploadMutex};
			p_ctx->taskQueue.push(upload_task);
			p_ctx->cv.notify_one();
		}
	}

	auto upload_data_to_texture(UploadContext *p_ctx, const TextureUploadDesc &p_upload_desc, StateTrackerHandle p_state_tracker) -> void
	{
		TST_ASSERT(p_ctx->stateTrackers.isValid(p_state_tracker));

		UploadTask upload_task{};
		allocateAcrossPages(p_ctx, p_upload_desc.size, upload_task.stagingAllocation);

		std::memcpy(upload_task.stagingAllocation.mapped, p_upload_desc.data, p_upload_desc.size);

		upload_task.extent       = p_upload_desc.extent;
		upload_task.offset       = p_upload_desc.offset;
		upload_task.stateTracker = p_state_tracker;
		upload_task.size         = p_upload_desc.size;
		upload_task.handle       = static_cast<uint64>(p_upload_desc.dstTexture);
		upload_task.type         = UploadTask::EType::eTexture;

		{
			std::unique_lock<std::mutex> lock{p_ctx->uploadMutex};
			p_ctx->taskQueue.push(upload_task);
			p_ctx->cv.notify_one();
		}
	}

	auto _get_transfer_timeline_semaphore(UploadContext *p_ctx) -> SemaphoreHandle
	{
		return p_ctx->transferTimelineSemaphore;
	}

	auto _acquire_transfer_timeline_counter_value(UploadContext *p_ctx) -> uint64
	{
		return ++p_ctx->transferTimelineCounter; // Pre inc
	}

	auto _get_transfer_timeline_counter_value(UploadContext *p_ctx) -> uint64
	{
		return p_ctx->transferTimelineCounter.load();
	}
}
