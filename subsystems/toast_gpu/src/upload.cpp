#include "toast_gpu/upload.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <threads.h>
#include <variant>

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/api.hpp"
#include "toast_lib/atomic.hpp"
#include "toast_lib/map.hpp"
#include "toast_lib/pool.hpp"
#include "toast_lib/queue.hpp"
#include "toast_lib/set.hpp"

namespace toaster::gpu
{
	struct StateTracker
	{
		mtx_t                    ticketMutex;
		Vector<uint64>           timelineTickets;
		StateTrackerReadyFn      readyCb{nullptr};
		TST_ALIGN_ATOMIC(uint32) pendingUploads{0u};
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

	// "Standard" boilerplate for visiting variants
	template<class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};

	template<class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

	struct BufferUploadTask
	{
		Vector<BufferHandle>              dstBuffers;
		Vector<Vector<BufferCopyRegion> > uploadRegions;
	};

	struct TextureUploadTask
	{
		Vector<TextureHandle>                    dstTextures;
		Vector<Vector<BufferTextureCopyRegion> > uploadRegions;
	};

	struct UploadTask
	{
		UploadPageAllocation                              stagingAllocation{};
		StateTrackerHandle                                stateTracker{nullptr};
		std::variant<BufferUploadTask, TextureUploadTask> task;
	};

	struct LiveUploadBatch
	{
		Vector<UploadPageAllocation> allocations;
		uint64                       timelineValue{0u};
	};

	struct UploadContext
	{
		GPUContext *       gpuCtx{nullptr};
		AllocationContext *allocCtx{nullptr};

		SemaphoreHandle     transferTimelineSemaphore{nullptr};
		std::atomic<uint64> transferTimelineCounter{0u};

		uint32                            nextPageId{0u};
		uint32                            activePageId{0u};
		UnorderedMap<uint32, StagingPage> stagingPages;

		FixedPool<StateTracker, 32u, 32u> stateTrackers; // 1024
		UnorderedSet<StateTrackerHandle>  aliveStateTrackers;

		uint32                         maxAllocationCommandLists{3u};
		Vector<CommandAllocatorHandle> commandAllocators;
		Vector<CommandListHandle>      commandLists;
		Vector<uint64>                 uploadTimeline;

		mtx_t aliveStateTrackerMtx;
		mtx_t pageMtx;
		mtx_t uploadMtx;
		mtx_t batchMtx;

		thrd_t           uploadThrd;
		std::atomic_bool threadRunning{true};
		cnd_t            cnd;

		Queue<UploadTask>       taskQueue;
		Vector<LiveUploadBatch> liveBatches;
	};

	static auto createPage(const UploadContext *p_ctx, uint64 p_size, uint32 p_id) -> StagingPage
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

	static auto destroyPage(const UploadContext *p_ctx, StagingPage &p_page) -> void
	{
		virtual_block_destroy(p_ctx->allocCtx, p_page.block);
		buffer_destroy(p_ctx->gpuCtx, p_page.buffer);
	}

	static auto tryAllocate(const UploadContext *p_ctx, StagingPage &p_page, uint64 p_size, uint64 p_alignment, UploadPageAllocation &p_out_allocation) -> bool
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

	static auto freeUploadPageAllocation(const UploadContext *p_ctx, StagingPage &p_page, const UploadPageAllocation &p_allocation) -> void
	{
		virtual_block_free(p_ctx->allocCtx, p_allocation.allocation);
		TST_SCOPED_ATOMIC(p_page.activeAllocations, active_allocations);
		--active_allocations;
	}

	static auto insertPage(UploadContext *p_ctx) -> void
	{
		const uint32 page_id{++p_ctx->nextPageId};
		p_ctx->stagingPages[page_id] = createPage(p_ctx, page_size, page_id);
		p_ctx->activePageId          = page_id;
	}

	static auto allocateAcrossPages(UploadContext *p_ctx, uint64 p_size, UploadPageAllocation &p_out_allocation) -> void
	{
		mtx_lock(&p_ctx->pageMtx);

		if (tryAllocate(p_ctx, p_ctx->stagingPages[p_ctx->activePageId], p_size, 16u, p_out_allocation))
		{
			mtx_unlock(&p_ctx->pageMtx);
			return;
		}

		for (auto &page: p_ctx->stagingPages | std::views::values)
		{
			if (tryAllocate(p_ctx, page, p_size, 16u, p_out_allocation))
			{
				mtx_unlock(&p_ctx->pageMtx);
				return;
			}
		}

		if (p_size > page_size)
		{
			const uint32 page_id{++p_ctx->nextPageId};
			p_ctx->stagingPages[page_id] = createPage(p_ctx, p_size, page_id);

			if (!tryAllocate(p_ctx, p_ctx->stagingPages[page_id], p_size, 16u, p_out_allocation))
				TST_PERMA_ASSERT(false);

			mtx_unlock(&p_ctx->pageMtx);
			return;
		}

		insertPage(p_ctx);
		if (!tryAllocate(p_ctx, p_ctx->stagingPages[p_ctx->activePageId], p_size, 16u, p_out_allocation))
		{
			TST_PERMA_ASSERT(false);
		}

		mtx_unlock(&p_ctx->pageMtx);
	}

	static auto executeBatchSubmission(UploadContext *p_ctx, const Vector<UploadTask> &p_tasks, uint32 p_cmd_index) -> void
	{
		const uint64            current_value{semaphore_get_value(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore)};
		const CommandListHandle cmd{p_ctx->commandLists[p_cmd_index]};

		if (current_value < p_ctx->uploadTimeline[p_cmd_index])
			semaphore_wait_n(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore, p_ctx->uploadTimeline[p_cmd_index]);

		cmd_allocator_reset(p_ctx->gpuCtx, p_ctx->commandAllocators[p_cmd_index]);
		cmd_list_open(p_ctx->gpuCtx, cmd);

		LiveUploadBatch live_batch{};

		for (const auto &task: p_tasks)
		{
			live_batch.allocations.push_back(task.stagingAllocation);

			std::visit(overloaded{
						   [&task, p_ctx, cmd](const BufferUploadTask &p_buffer_task) -> void
						   {
							   for (uint32 i{0u}; i < p_buffer_task.uploadRegions.size(); ++i)
							   {
								   BufferHandle dst_buffer{p_buffer_task.dstBuffers[i]};
								   auto &       upload_regions{p_buffer_task.uploadRegions[i]};
								   cmd_copy_buffer(p_ctx->gpuCtx, cmd, task.stagingAllocation.buffer, dst_buffer, upload_regions);
							   }
						   },
						   [&task, p_ctx, cmd](const TextureUploadTask &p_texture_task) -> void
						   {
							   for (uint32 i{0u}; i < p_texture_task.uploadRegions.size(); ++i)
							   {
								   TextureHandle dst_texture{p_texture_task.dstTextures[i]};
								   auto &        upload_regions{p_texture_task.uploadRegions[i]};
								   cmd_copy_buffer_to_texture(p_ctx->gpuCtx, cmd, task.stagingAllocation.buffer, dst_texture, upload_regions);
							   }
						   }
					   }, task.task);
		}

		cmd_list_close(p_ctx->gpuCtx, cmd);

		const uint64 target_signal_value{_acquire_transfer_timeline_counter_value(p_ctx)};
		p_ctx->uploadTimeline[p_cmd_index] = target_signal_value;
		live_batch.timelineValue           = target_signal_value;

		queue_submit(p_ctx->gpuCtx, EQueueType::eTransfer, cmd, {}, SemaphoreSubmitInfo{p_ctx->transferTimelineSemaphore, target_signal_value});

		for (const auto &task: p_tasks)
		{
			StateTracker &tracker{p_ctx->stateTrackers[task.stateTracker]};

			mtx_lock(&tracker.ticketMutex);
			tracker.timelineTickets.push_back(target_signal_value);
			mtx_unlock(&tracker.ticketMutex);

			TST_SCOPED_ATOMIC(tracker.pendingUploads, pending_uploads);
			(void) --pending_uploads;
		}

		mtx_lock(&p_ctx->batchMtx);
		p_ctx->liveBatches.push_back(live_batch);
		mtx_unlock(&p_ctx->batchMtx);
	}

	static auto __cdecl upload_main(void *p_arg) -> int32
	{
		const auto ctx{static_cast<UploadContext *>(p_arg)};

		uint32 cmd_index{0u};

		while (ctx->threadRunning)
		{
			Vector<UploadTask> batch_tasks;

			{
				mtx_lock(&ctx->uploadMtx);
				while (ctx->taskQueue.empty() && ctx->threadRunning.load())
					cnd_wait(&ctx->cnd, &ctx->uploadMtx);

				if (ctx->taskQueue.empty() && !ctx->threadRunning.load())
				{
					mtx_unlock(&ctx->uploadMtx);
					break;
				}

				while (!ctx->taskQueue.empty())
				{
					batch_tasks.push_back(ctx->taskQueue.front());
					ctx->taskQueue.pop();
				}
				mtx_unlock(&ctx->uploadMtx);
			}

			if (!batch_tasks.empty())
			{
				executeBatchSubmission(ctx, batch_tasks, cmd_index);
				cmd_index = (cmd_index + 1u) % ctx->maxAllocationCommandLists;
			}
		}

		return 67;
	}

	auto upload_ctx_create(GPUContext *p_gpu_ctx, AllocationContext *p_alloc_ctx, const UploadContextDesc &p_desc) -> UploadContext *
	{
		const auto ctx{new UploadContext{}};

		ctx->gpuCtx   = p_gpu_ctx;
		ctx->allocCtx = p_alloc_ctx;

		mtx_init(&ctx->aliveStateTrackerMtx, mtx_plain);
		mtx_init(&ctx->pageMtx, mtx_plain);
		mtx_init(&ctx->uploadMtx, mtx_plain);
		mtx_init(&ctx->batchMtx, mtx_plain);

		cnd_init(&ctx->cnd);

		ctx->transferTimelineSemaphore = semaphore_create(p_gpu_ctx);
		ctx->maxAllocationCommandLists = p_desc.maxAllocationCommandLists;

		insertPage(ctx); // Default page

		ctx->commandAllocators.resize(ctx->maxAllocationCommandLists);
		ctx->commandLists.resize(ctx->maxAllocationCommandLists);
		ctx->uploadTimeline.resize(ctx->maxAllocationCommandLists);
		for (uint32 i{0u}; i < ctx->maxAllocationCommandLists; ++i)
		{
			ctx->uploadTimeline[i]    = 0u;
			ctx->commandAllocators[i] = cmd_allocator_create(p_gpu_ctx, EQueueType::eTransfer);
			cmd_list_allocate_n(p_gpu_ctx, ctx->commandAllocators[i], std::span<CommandListHandle>{&ctx->commandLists[i], 1ull}, false);
		}

		ctx->stateTrackers.setDestructorFn(+[](StateTracker *p_data, void *) -> void
		{
			mtx_destroy(&p_data->ticketMutex);
			p_data->timelineTickets.clear();
			p_data->pendingUploads = 0u;
			p_data->readyCb        = nullptr;
		});

		if (thrd_create(&ctx->uploadThrd, upload_main, ctx) != thrd_success)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to create upload thread!");
			return nullptr;
		}

		return ctx;
	}

	auto upload_ctx_destroy(UploadContext *p_ctx) -> void
	{
		p_ctx->threadRunning.store(false);
		cnd_signal(&p_ctx->cnd);

		int32 _res;
		thrd_join(p_ctx->uploadThrd, &_res);
		if (_res != 67)
		{
			TST_PERMA_ASSERT_MSG(false, "Thread failed to join");
		}
		cnd_destroy(&p_ctx->cnd);

		p_ctx->stateTrackers.clear();
		p_ctx->aliveStateTrackers.clear(); // They are now all dead

		for (uint32 i{0u}; i < p_ctx->maxAllocationCommandLists; ++i)
			cmd_allocator_destroy(p_ctx->gpuCtx, p_ctx->commandAllocators[i]);

		for (auto &page: p_ctx->stagingPages | std::views::values)
			destroyPage(p_ctx, page);

		semaphore_destroy(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore);

		mtx_destroy(&p_ctx->batchMtx);
		mtx_destroy(&p_ctx->uploadMtx);
		mtx_destroy(&p_ctx->pageMtx);
		mtx_destroy(&p_ctx->aliveStateTrackerMtx);

		delete p_ctx;
	}

	auto state_tracker_register(UploadContext *p_ctx, uint32 p_expected_uploads) -> StateTrackerHandle
	{
		StateTracker state_tracker{};
		mtx_init(&state_tracker.ticketMutex, mtx_plain);
		state_tracker.pendingUploads = p_expected_uploads;
		StateTrackerHandle out_handle{p_ctx->stateTrackers.emplace(std::move(state_tracker))};

		mtx_lock(&p_ctx->aliveStateTrackerMtx);
		p_ctx->aliveStateTrackers.insert(out_handle);
		mtx_unlock(&p_ctx->aliveStateTrackerMtx);

		return out_handle;
	}

	auto state_tracker_register_ready_callback(UploadContext *p_ctx, StateTrackerHandle p_state_tracker, StateTrackerReadyFn p_ready_callback) -> void
	{
		StateTracker *tracker{p_ctx->stateTrackers.try_get(p_state_tracker)};
		TST_ASSERT_MSG(tracker, "State tracker does not exist or is already completed");
		tracker->readyCb = p_ready_callback;
	}

	auto state_tracker_is_ready(const UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> bool
	{
		TST_ASSERT_MSG(p_state_tracker, "State tracker is nullptr, so it is not ready!");
		// If the state tracker is valid, it means that it has not completed all of its operations
		return !p_ctx->stateTrackers.is_valid(p_state_tracker);
	}

	auto state_tracker_wait_for(UploadContext *p_ctx, StateTrackerHandle p_state_tracker) -> void
	{
		StateTracker *tracker{p_ctx->stateTrackers.try_get(p_state_tracker)};
		if (!tracker)
			return;

		while (true)
		{
			const uint64 current_value{semaphore_get_value(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore)};

			bool all_finished{true};

			mtx_lock(&tracker->ticketMutex);

			// If the tracker has no timeline tickets, it indicates that worker threads have not yet begun an upload on it
			if (tracker->timelineTickets.empty())
				all_finished = false;
			else
			{
				// If any of the timeline tickets are greater than the current value, we are not "all finished"
				all_finished = !std::ranges::any_of(tracker->timelineTickets, [current_value](const uint64 p_ticket)-> bool { return current_value < p_ticket; });
			}

			mtx_unlock(&tracker->ticketMutex);

			TST_SCOPED_ATOMIC(tracker->pendingUploads, pending_uploads);
			if (all_finished && pending_uploads.load() == 0u)
			{
				if (tracker->readyCb)
					tracker->readyCb();

				p_ctx->stateTrackers.destroy(p_state_tracker);

				mtx_lock(&p_ctx->aliveStateTrackerMtx);
				p_ctx->aliveStateTrackers.erase(p_state_tracker);
				mtx_unlock(&p_ctx->aliveStateTrackerMtx);

				break;
			}
		}
	}

	auto poll_uploads(UploadContext *p_ctx) -> void
	{
		const uint64 current_value{semaphore_get_value(p_ctx->gpuCtx, p_ctx->transferTimelineSemaphore)};

		mtx_lock(&p_ctx->batchMtx);
		mtx_lock(&p_ctx->pageMtx);

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
		mtx_unlock(&p_ctx->pageMtx);
		mtx_unlock(&p_ctx->batchMtx);

		Vector<StateTrackerHandle> handles_to_remove;

		mtx_lock(&p_ctx->aliveStateTrackerMtx);
		for (auto it{p_ctx->aliveStateTrackers.begin()}; it != p_ctx->aliveStateTrackers.end();)
		{
			StateTracker &data{p_ctx->stateTrackers[*it]};

			bool all_finished{true};

			mtx_lock(&data.ticketMutex);
			// If the tracker has no timeline tickets, it indicates that worker threads have not yet begun an upload on it
			if (data.timelineTickets.empty())
				all_finished = false;
			else
			{
				// If any of the timeline tickets are greater than the current value, we are not "all finished"
				all_finished = !std::ranges::any_of(data.timelineTickets, [current_value](const uint64 p_ticket)-> bool { return current_value < p_ticket; });
			}

			mtx_unlock(&data.ticketMutex);

			TST_SCOPED_ATOMIC(data.pendingUploads, pending_uploads);
			if (all_finished && pending_uploads.load() == 0u)
			{
				if (data.readyCb)
					data.readyCb();

				handles_to_remove.push_back(*it); // The state tracker is ready, so it can be destroyed
				it = p_ctx->aliveStateTrackers.erase(it);
			}
			else
				++it;
		}
		mtx_unlock(&p_ctx->aliveStateTrackerMtx);

		if (!handles_to_remove.empty())
			p_ctx->stateTrackers.destroy_n(handles_to_remove);
	}

	auto upload_data_to_buffers(UploadContext *    p_ctx, InitialiserList<const BufferHandle> p_dst_buffers, InitialiserList<const BufferUploadDesc> p_upload_descs,
								StateTrackerHandle p_state_tracker) -> void
	{
		TST_PERMA_ASSERT_MSG(p_ctx->stateTrackers.is_valid(p_state_tracker), "Attempted to upload data to texture using invalid state tracker");

		UploadTask upload_task{};

		uint64 total_size{0ull};
		for (const BufferUploadDesc &desc: p_upload_descs)
		{
			total_size += desc.size;
			total_size = TST_ALIGN(total_size, 16u);
		}

		allocateAcrossPages(p_ctx, total_size, upload_task.stagingAllocation);

		BufferUploadTask &task{upload_task.task.emplace<BufferUploadTask>()};
		task.dstBuffers.assign_range(p_dst_buffers);
		task.uploadRegions.resize(p_upload_descs.size());

		uintptr offset{0ull};
		for (uint32 i{0u}; i < p_upload_descs.size(); ++i)
		{
			const BufferUploadDesc &desc{p_upload_descs[i]};

			std::memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr>(upload_task.stagingAllocation.mapped) + offset), desc.data, desc.size);

			auto &region_vec{task.uploadRegions[i]};
			region_vec.assign_range(desc.uploadRegions);

			for (auto &region: region_vec)
				region.srcOffset += upload_task.stagingAllocation.offset + offset; // The offset has to be relative to the staging allocation

			offset += desc.size;
			offset = TST_ALIGN(offset, 16u); // Now it is "safe"
		}

		upload_task.stateTracker = p_state_tracker;

		mtx_lock(&p_ctx->uploadMtx);
		p_ctx->taskQueue.push(upload_task);
		cnd_signal(&p_ctx->cnd);
		mtx_unlock(&p_ctx->uploadMtx);
	}

	auto upload_data_to_textures(UploadContext *    p_ctx, InitialiserList<const TextureHandle> p_dst_textures, InitialiserList<const TextureUploadDesc> p_upload_descs,
								 StateTrackerHandle p_state_tracker) -> void
	{
		TST_PERMA_ASSERT_MSG(p_ctx->stateTrackers.is_valid(p_state_tracker), "Attempted to upload data to texture using invalid state tracker");

		UploadTask upload_task{};

		uint64 total_size{0ull};
		for (const TextureUploadDesc &desc: p_upload_descs)
		{
			total_size += desc.size;
			total_size = TST_ALIGN(total_size, 16u);
		}

		allocateAcrossPages(p_ctx, total_size, upload_task.stagingAllocation);

		TextureUploadTask &task{upload_task.task.emplace<TextureUploadTask>()};
		task.dstTextures.assign_range(p_dst_textures);
		task.uploadRegions.resize(p_upload_descs.size());

		uintptr offset{0ull};
		for (uint32 i{0u}; i < p_upload_descs.size(); ++i)
		{
			const TextureUploadDesc &desc{p_upload_descs[i]};

			std::memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr>(upload_task.stagingAllocation.mapped) + offset), desc.data, desc.size);

			auto &region_vec{task.uploadRegions[i]};
			region_vec.assign_range(desc.uploadRegions);

			for (auto &region: region_vec)
				region.bufferOffset += upload_task.stagingAllocation.offset + offset; // The offset has to be relative to the staging allocation

			offset += desc.size;
			offset = TST_ALIGN(offset, 16u); // Now it is "safe"
		}

		upload_task.stateTracker = p_state_tracker;

		mtx_lock(&p_ctx->uploadMtx);
		p_ctx->taskQueue.push(upload_task);
		cnd_signal(&p_ctx->cnd);
		mtx_unlock(&p_ctx->uploadMtx);
	}

	auto _get_transfer_timeline_semaphore(const UploadContext *p_ctx) -> SemaphoreHandle
	{
		return p_ctx->transferTimelineSemaphore;
	}

	auto _acquire_transfer_timeline_counter_value(UploadContext *p_ctx) -> uint64
	{
		return ++p_ctx->transferTimelineCounter; // Pre inc
	}

	auto _get_transfer_timeline_counter_value(const UploadContext *p_ctx) -> uint64
	{
		return p_ctx->transferTimelineCounter.load();
	}
}
