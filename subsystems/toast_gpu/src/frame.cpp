#include "toast_gpu/frame.hpp"

namespace toaster::gpu::frame
{
	struct DeferredDeletion
	{
		enum class EDeferredDeletionType : uint8
		{
			eTexture,
			eBuffer,
			eBufferHeapSlot,
			eTextureHeapSlot
		};

		uint64 handle{0u};
		uint64 graphicsTimelineValue{0u};
		uint64 transferTimelineValue{0u};

		EDeferredDeletionType type;

		ResourceDescriptorHeapHandle descriptorHeap{nullptr}; // gpu::ResourceDescriptorHeapHandle or gpu::SamplerDescriptorHeapHandle
		uint32                       heapSlot{UINT32_MAX};
	};

	struct FrameContextImpl
	{
		uint32 maxFramesInFlight{3u};
		uint32 currentFrameIndex{0u};

		SemaphoreHandle     graphicsTimelineSemaphore{nullptr};
		uint64              graphicsTimelineCounter{0u};
		std::vector<uint64> graphicsTimelineValues;

		SemaphoreHandle transferTimelineSemaphore{nullptr};
		uint64          transferTimelineCounter{0u};

		std::vector<DeferredDeletion> deferredDeletions;
	};

	static FrameContextImpl *g_impl{nullptr};

	auto deleteDeferredDeletion(DeferredDeletion &p_dd) -> void
	{
		switch (p_dd.type)
		{
			case DeferredDeletion::EDeferredDeletionType::eBuffer:
			{
				destroyBuffer(BufferHandle(p_dd.handle));
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eTexture:
			{
				destroyTexture(TextureHandle(p_dd.handle));
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eBufferHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					freeBufferHeapSlot(p_dd.descriptorHeap, p_dd.heapSlot);
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eTextureHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					freeTextureHeapSlot(p_dd.descriptorHeap, p_dd.heapSlot);
				break;
			}
		}
	}

	auto initFrameContext(const FrameContextDesc &p_desc) -> void
	{
		if (g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Frame context has already been initialised!");
			return;
		}

		g_impl = new FrameContextImpl{};

		g_impl->maxFramesInFlight = p_desc.maxFramesInFlight;

		g_impl->graphicsTimelineSemaphore = createSemaphore();
		g_impl->transferTimelineSemaphore = createSemaphore();
		g_impl->graphicsTimelineValues.resize(g_impl->maxFramesInFlight);
		for (uint32 f{0u}; f < g_impl->maxFramesInFlight; ++f)
			g_impl->graphicsTimelineValues[f] = 0u;
	}

	auto shutdownFrameContext() -> void
	{
		if (!g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "Frame context has not been initialised!");
			return;
		}

		waitIdle();

		for (auto &dd: g_impl->deferredDeletions)
			deleteDeferredDeletion(dd);
		g_impl->deferredDeletions.clear();

		destroySemaphore(g_impl->graphicsTimelineSemaphore);
		destroySemaphore(g_impl->transferTimelineSemaphore);

		delete g_impl;
		g_impl = nullptr;
	}

	auto getMaxFramesInFlight() -> uint32
	{
		return g_impl->maxFramesInFlight;
	}

	auto processDeferredDeletions(uint32 p_frame_index) -> void
	{
		const uint64 completedTransferValue{getSemaphoreValue(g_impl->transferTimelineSemaphore)};
		for (auto it{g_impl->deferredDeletions.begin()}; it != g_impl->deferredDeletions.end();)
		{
			if (g_impl->graphicsTimelineValues[p_frame_index] >= it->graphicsTimelineValue && completedTransferValue >= it->transferTimelineValue)
			{
				deleteDeferredDeletion(*it);

				it = g_impl->deferredDeletions.erase(it);
			}
			else
				++it;
		}
	}

	auto beginFrame(uint32 p_frame_index) -> void
	{
		p_frame_index             %= g_impl->maxFramesInFlight; // Wrap it to the max frames in flight
		g_impl->currentFrameIndex = p_frame_index;

		if (g_impl->graphicsTimelineValues[p_frame_index] > 0u)
			waitSemaphores(g_impl->graphicsTimelineSemaphore, g_impl->graphicsTimelineValues[p_frame_index]);

		processDeferredDeletions(p_frame_index);
	}

	auto submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list) -> bool
	{
		++g_impl->graphicsTimelineCounter;
		g_impl->graphicsTimelineValues[g_impl->currentFrameIndex] = g_impl->graphicsTimelineCounter;

		// std::vector<SemaphoreSubmitInfo> waits;
		// if (g_impl->transferTimelineCounter > 0u)
		// waits.emplace_back(SemaphoreSubmitInfo{g_impl->transferTimelineSemaphore, g_impl->transferTimelineCounter}); // Wait on the transfer queue

		const bool success{gpu::submitAndPresent(p_swapchain, p_command_list, {g_impl->graphicsTimelineSemaphore, g_impl->graphicsTimelineCounter})};
		return success;
	}

	auto getTransferTimelineSemaphore() -> SemaphoreHandle
	{
		return g_impl->transferTimelineSemaphore;
	}

	auto acquireTransferTimelineCounterValue() -> uint64
	{
		return ++g_impl->transferTimelineCounter; // Pre inc
	}

	auto getTransferTimelineCounterValue() -> uint64
	{
		return g_impl->transferTimelineCounter;
	}

	auto defferBufferDeletion(BufferHandle p_buffer) -> void
	{
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   static_cast<uint64>(p_buffer),
												   g_impl->graphicsTimelineCounter,
												   g_impl->transferTimelineCounter,
												   DeferredDeletion::EDeferredDeletionType::eBuffer
											   });
	}

	auto defferTextureDeletion(TextureHandle p_texture) -> void
	{
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   static_cast<uint64>(p_texture),
												   g_impl->graphicsTimelineCounter,
												   g_impl->transferTimelineCounter,
												   DeferredDeletion::EDeferredDeletionType::eTexture
											   });
	}

	auto defferBufferSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_slot) -> void
	{
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   0u,
												   g_impl->graphicsTimelineCounter,
												   g_impl->transferTimelineCounter,
												   DeferredDeletion::EDeferredDeletionType::eBufferHeapSlot,
												   p_resource_heap,
												   p_slot
											   });
	}

	auto defferTextureSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_slot) -> void
	{
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   0u,
												   g_impl->graphicsTimelineCounter,
												   g_impl->transferTimelineCounter,
												   DeferredDeletion::EDeferredDeletionType::eTextureHeapSlot,
												   p_resource_heap,
												   p_slot
											   });
	}
}
