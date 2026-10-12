#pragma once

#include "toast_render.hpp"
#include "toast_gpu/api.hpp"

namespace toaster::render
{
	// Initialises everything to do with the gpu
	class TST_RENDER_API RenderContext
	{
	public:
		RenderContext(uint32 p_max_frames_in_flight    = 3u, uint32 p_max_buffer_descriptors = 128u, uint32 p_max_texture_descriptors = 1028u,
					  uint32 p_max_sampler_descriptors = 32u);
		~RenderContext();

		[[nodiscard]] auto getResourceHeap() const -> gpu::ResourceDescriptorHeapHandle { return m_resourceHeap; }
		[[nodiscard]] auto getSamplerHeap() const -> gpu::SamplerDescriptorHeapHandle { return m_samplerHeap; }

	private:
		gpu::ResourceDescriptorHeapHandle m_resourceHeap{nullptr};
		gpu::SamplerDescriptorHeapHandle  m_samplerHeap{nullptr};
	};
}
