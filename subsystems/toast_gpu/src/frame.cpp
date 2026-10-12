#include "toast_gpu/frame.hpp"

#include <mutex>

namespace toaster::gpu::frame
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

	struct FrameContextImpl
	{
		uint32 maxFramesInFlight{3u};
		uint32 currentFrameIndex{0u};

		SemaphoreHandle      graphicsTimelineSemaphore{nullptr};
		std::atomic_uint64_t graphicsTimelineCounter{0u};
		std::vector<uint64>  graphicsTimelineValues;

		std::vector<DeferredDeletion> deferredDeletions;
		std::mutex                    deferredDeletionMutex;
	};

	static FrameContextImpl *g_impl{nullptr};

	static auto deleteDeferredDeletion(DeferredDeletion &p_dd) -> void
	{
		switch (p_dd.type)
		{
			case DeferredDeletion::EDeferredDeletionType::eBuffer:
			{
				destroyBuffer(static_cast<BufferHandle>(p_dd.handle));
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eTexture:
			{
				destroyTexture(static_cast<TextureHandle>(p_dd.handle));
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eBufferHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					freeBufferHeapSlot(static_cast<ResourceDescriptorHeapHandle>(p_dd.descriptorHeap), p_dd.heapSlot);
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eTextureHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					freeTextureHeapSlot(static_cast<ResourceDescriptorHeapHandle>(p_dd.descriptorHeap), p_dd.heapSlot);
				break;
			}
			case DeferredDeletion::EDeferredDeletionType::eSamplerHeapSlot:
			{
				if (p_dd.heapSlot != UINT32_MAX)
					freeSamplerHeapSlot(static_cast<SamplerDescriptorHeapHandle>(p_dd.descriptorHeap), p_dd.heapSlot);
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

		delete g_impl;
		g_impl = nullptr;
	}

	auto getMaxFramesInFlight() -> uint32
	{
		return g_impl->maxFramesInFlight;
	}

	auto beginFrame(uint32 p_frame_index) -> void
	{
		p_frame_index             %= g_impl->maxFramesInFlight; // Wrap it to the max frames in flight
		g_impl->currentFrameIndex = p_frame_index;

		if (g_impl->graphicsTimelineValues[p_frame_index] > 0u)
			waitSemaphores(g_impl->graphicsTimelineSemaphore, g_impl->graphicsTimelineValues[p_frame_index]);

		processDeferredDeletions();
	}

	auto processDeferredDeletions() -> void
	{
		const uint64 current_value{getGraphicsTimelineSemaphoreCurrentValue()};

		std::scoped_lock<std::mutex> lock{g_impl->deferredDeletionMutex};
		for (auto it{g_impl->deferredDeletions.begin()}; it != g_impl->deferredDeletions.end();)
		{
			if (current_value >= it->graphicsTimelineValue)
			{
				deleteDeferredDeletion(*it);
				it = g_impl->deferredDeletions.erase(it);
			}
			else
				++it;
		}
	}

	auto submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list) -> bool
	{
		const uint64 counter_value{++g_impl->graphicsTimelineCounter};
		g_impl->graphicsTimelineValues[g_impl->currentFrameIndex] = counter_value;

		return gpu::submitAndPresent(p_swapchain, p_command_list, {g_impl->graphicsTimelineSemaphore, counter_value});
	}

	auto getGraphicsTimelineSemaphoreCurrentValue() -> uint64
	{
		return getSemaphoreValue(g_impl->graphicsTimelineSemaphore);
	}

	auto deferBufferDeletion(BufferHandle p_buffer) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->deferredDeletionMutex};
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   static_cast<uint64>(p_buffer),
												   g_impl->graphicsTimelineCounter.load(),
												   DeferredDeletion::EDeferredDeletionType::eBuffer
											   });
	}

	auto deferTextureDeletion(TextureHandle p_texture) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->deferredDeletionMutex};
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   static_cast<uint64>(p_texture),
												   g_impl->graphicsTimelineCounter.load(),
												   DeferredDeletion::EDeferredDeletionType::eTexture
											   });
	}

	auto deferBufferHeapSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_buffer_slot) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->deferredDeletionMutex};
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   0u,
												   g_impl->graphicsTimelineCounter.load(),
												   DeferredDeletion::EDeferredDeletionType::eBufferHeapSlot,
												   static_cast<uint64>(p_resource_heap),
												   p_buffer_slot
											   });
	}

	auto deferTextureHeapSlotFreeing(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_texture_slot) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->deferredDeletionMutex};
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   0u,
												   g_impl->graphicsTimelineCounter.load(),
												   DeferredDeletion::EDeferredDeletionType::eTextureHeapSlot,
												   static_cast<uint64>(p_resource_heap),
												   p_texture_slot
											   });
	}

	auto deferSamplerHeapSlotFreeing(SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_sampler_slot) -> void
	{
		std::scoped_lock<std::mutex> lock{g_impl->deferredDeletionMutex};
		g_impl->deferredDeletions.emplace_back(DeferredDeletion{
												   0u,
												   g_impl->graphicsTimelineCounter.load(),
												   DeferredDeletion::EDeferredDeletionType::eSamplerHeapSlot,
												   static_cast<uint64>(p_sampler_heap),
												   p_sampler_slot
											   });
	}
}
