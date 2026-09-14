#pragma once

#include "toast_render.hpp"
#include "toast_gpu/api.hpp"

namespace toaster::render
{
	// Initialises everything to do with the gpu
	class TST_RENDER_API RenderContext
	{
	public:
		RenderContext();
		~RenderContext();

		[[nodiscard]] auto getResourceHeap() const -> gpu::ResourceDescriptorHeapHandle { return m_resourceHeap; }
		[[nodiscard]] auto getSamplerHeap() const -> gpu::SamplerDescriptorHeapHandle { return m_samplerHeap; }

	private:
		gpu::ResourceDescriptorHeapHandle m_resourceHeap{nullptr};
		gpu::SamplerDescriptorHeapHandle  m_samplerHeap{nullptr};
	};
}
