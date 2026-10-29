#include "toast_gpu/frame.hpp"
#include "toast_gpu/api.hpp"

#include <mutex>
#include <threads.h>

namespace toaster::gpu
{
	struct DeferredDeletion
	{
		enum class EDeferredDeletionType : uint8
		{
			eTexture,
			eBuffer,
			eBufferHeapSlot,
			eTextureHeapSlot,
			eSamplerHeapSlot
		};

		uint64 handle{0u};
		uint64 graphicsTimelineValue{0u};

		EDeferredDeletionType type;

		uint64 descriptorHeap{0u}; // gpu::ResourceDescriptorHeapHandle or gpu::SamplerDescriptorHeapHandle
		uint32 heapSlot{UINT32_MAX};
	};

	struct FrameContext
	{
		GPUContext *gpuCtx{nullptr};

		uint32 maxFramesInFlight{3u};
		uint32 currentFrameIndex{0u};

		SemaphoreHandle      graphicsTimelineSemaphore{nullptr};
		std::atomic_uint64_t graphicsTimelineCounter{0u};
		Vector<uint64>       graphicsTimelineValues;

		Vector<DeferredDeletion> deferredDeletions;
		mtx_t                    deferredDeletionMutex;
	};

	static auto delete_deferred_deletion(FrameContext *p_ctx, DeferredDeletion &p_dd) -> void
	{
		switch (p_dd.type)
		{
			case DeferredDeletion::EDeferredDeletionType::eBuffer:
			{
				buffer_destroy(p_ctx->gpuCtx, static_cast<BufferHandle>(p_dd.handle));
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eTexture:
			{
				texture_destroy(p_ctx->gpuCtx, static_cast<TextureHandle>(p_dd.handle));
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eBufferHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					resource_descriptor_heap_free_buffer_slot(p_ctx->gpuCtx, static_cast<ResourceDescriptorHeapHandle>(p_dd.descriptorHeap), p_dd.heapSlot);
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eTextureHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					resource_descriptor_heap_free_texture_slot(p_ctx->gpuCtx, static_cast<ResourceDescriptorHeapHandle>(p_dd.descriptorHeap), p_dd.heapSlot);
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eSamplerHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					sampler_descriptor_heap_free_sampler_slot(p_ctx->gpuCtx, static_cast<SamplerDescriptorHeapHandle>(p_dd.descriptorHeap), p_dd.heapSlot);
				break;
			}
		}
	}

	auto frame_ctx_create(GPUContext *p_gpu_ctx, const FrameContextDesc &p_desc) -> FrameContext *
	{
		FrameContext *ctx{new FrameContext{}};

		mtx_init(&ctx->deferredDeletionMutex, mtx_plain);

		ctx->gpuCtx            = p_gpu_ctx;
		ctx->maxFramesInFlight = p_desc.maxFramesInFlight;

		ctx->graphicsTimelineSemaphore = semaphore_create(p_gpu_ctx);
		ctx->graphicsTimelineValues.resize(ctx->maxFramesInFlight);
		for (uint32 f{0u}; f < ctx->maxFramesInFlight; ++f)
			ctx->graphicsTimelineValues[f] = 0u;

		return ctx;
	}

	auto frame_ctx_destroy(FrameContext *p_ctx) -> void
	{
		wait_idle(p_ctx->gpuCtx);

		for (auto &dd: p_ctx->deferredDeletions)
			delete_deferred_deletion(p_ctx, dd);
		p_ctx->deferredDeletions.clear();

		semaphore_destroy(p_ctx->gpuCtx, p_ctx->graphicsTimelineSemaphore);

		mtx_destroy(&p_ctx->deferredDeletionMutex);

		delete p_ctx;
	}

	auto frame_ctx_get_max_frames_in_flight(FrameContext *p_ctx) -> uint32
	{
		return p_ctx->maxFramesInFlight;
	}

	auto begin_frame(FrameContext *p_ctx, uint32 p_frame_index) -> void
	{
		p_frame_index            %= p_ctx->maxFramesInFlight; // Wrap it to the max frames in flight
		p_ctx->currentFrameIndex = p_frame_index;

		if (p_ctx->graphicsTimelineValues[p_frame_index] > 0u)
			semaphore_wait_n(p_ctx->gpuCtx, p_ctx->graphicsTimelineSemaphore, p_ctx->graphicsTimelineValues[p_frame_index]);

		process_deferred_deletions(p_ctx);
	}

	auto process_deferred_deletions(FrameContext *p_ctx) -> void
	{
		const uint64 current_value{get_graphics_timeline_semaphore_current_value(p_ctx)};

		mtx_lock(&p_ctx->deferredDeletionMutex);
		for (auto it{p_ctx->deferredDeletions.begin()}; it != p_ctx->deferredDeletions.end();)
		{
			if (current_value >= it->graphicsTimelineValue)
			{
				delete_deferred_deletion(p_ctx, *it);
				it = p_ctx->deferredDeletions.erase(it);
			}
			else
				++it;
		}
		mtx_unlock(&p_ctx->deferredDeletionMutex);
	}

	auto submit_and_present(FrameContext *p_ctx, SwapchainHandle p_swapchain, CommandListHandle p_command_list) -> bool
	{
		const uint64 counter_value{++p_ctx->graphicsTimelineCounter};
		p_ctx->graphicsTimelineValues[p_ctx->currentFrameIndex] = counter_value;

		return swapchain_submit_and_present(p_ctx->gpuCtx, p_swapchain, p_command_list, {p_ctx->graphicsTimelineSemaphore, counter_value});
	}

	auto get_graphics_timeline_semaphore_current_value(FrameContext *p_ctx) -> uint64
	{
		return semaphore_get_value(p_ctx->gpuCtx, p_ctx->graphicsTimelineSemaphore);
	}

	auto defer_buffer_deletion(FrameContext *p_ctx, BufferHandle p_buffer) -> void
	{
		mtx_lock(&p_ctx->deferredDeletionMutex);
		p_ctx->deferredDeletions.emplace_back(DeferredDeletion{
												  static_cast<uint64>(p_buffer),
												  p_ctx->graphicsTimelineCounter.load(),
												  DeferredDeletion::EDeferredDeletionType::eBuffer
											  });
		mtx_unlock(&p_ctx->deferredDeletionMutex);
	}

	auto defer_texture_deletion(FrameContext *p_ctx, TextureHandle p_texture) -> void
	{
		mtx_lock(&p_ctx->deferredDeletionMutex);
		p_ctx->deferredDeletions.emplace_back(DeferredDeletion{
												  static_cast<uint64>(p_texture),
												  p_ctx->graphicsTimelineCounter.load(),
												  DeferredDeletion::EDeferredDeletionType::eTexture
											  });
		mtx_unlock(&p_ctx->deferredDeletionMutex);
	}

	auto defer_buffer_heap_slot_freeing(FrameContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_buffer_slot) -> void
	{
		mtx_lock(&p_ctx->deferredDeletionMutex);
		p_ctx->deferredDeletions.emplace_back(DeferredDeletion{
												  0u,
												  p_ctx->graphicsTimelineCounter.load(),
												  DeferredDeletion::EDeferredDeletionType::eBufferHeapSlot,
												  static_cast<uint64>(p_resource_heap),
												  p_buffer_slot
											  });
		mtx_unlock(&p_ctx->deferredDeletionMutex);
	}

	auto defer_texture_heap_slot_freeing(FrameContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_texture_slot) -> void
	{
		mtx_lock(&p_ctx->deferredDeletionMutex);
		p_ctx->deferredDeletions.emplace_back(DeferredDeletion{
												  0u,
												  p_ctx->graphicsTimelineCounter.load(),
												  DeferredDeletion::EDeferredDeletionType::eTextureHeapSlot,
												  static_cast<uint64>(p_resource_heap),
												  p_texture_slot
											  });
		mtx_unlock(&p_ctx->deferredDeletionMutex);
	}

	auto defer_sampler_Heap_slot_freeing(FrameContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_sampler_slot) -> void
	{
		mtx_lock(&p_ctx->deferredDeletionMutex);
		p_ctx->deferredDeletions.emplace_back(DeferredDeletion{
												  0u,
												  p_ctx->graphicsTimelineCounter.load(),
												  DeferredDeletion::EDeferredDeletionType::eSamplerHeapSlot,
												  static_cast<uint64>(p_sampler_heap),
												  p_sampler_slot
											  });
		mtx_unlock(&p_ctx->deferredDeletionMutex);
	}
}
