#include "toast_render/render_context.hpp"

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	RenderContext::RenderContext(uint32 p_max_frames_in_flight, uint32 p_max_buffer_descriptors, uint32 p_max_texture_descriptors, uint32 p_max_sampler_descriptors)
	{
		gpu::GPUContextDesc gpu_context_desc{};
		gpu_context_desc.enableDebugInfo                 = true;
		gpu_context_desc.maxConcurrentSwapchainWorkloads = p_max_frames_in_flight;
		gpu_context_desc.usingSwapchain                  = true;
		gpu::initGPUContext(gpu_context_desc);

		gpu::frame::initFrameContext({p_max_frames_in_flight});
		gpu::alloc::initAllocationContext();
		gpu::upload::initUploadContext({});

		m_resourceHeap = gpu::createResourceDescriptorHeap({p_max_buffer_descriptors, p_max_texture_descriptors});
		m_samplerHeap  = gpu::createSamplerDescriptorHeap({p_max_sampler_descriptors});
	}

	RenderContext::~RenderContext()
	{
		gpu::upload::shutdownUploadContext();
		gpu::alloc::shutdownAllocationContext();
		gpu::frame::shutdownFrameContext();

		// Must happen after shutting down the frame context because some deferred deletions may depend on the descriptor heaps
		gpu::destroySamplerDescriptorHeap(m_samplerHeap);
		gpu::destroyResourceDescriptorHeap(m_resourceHeap);

		gpu::shutdownGPUContext();
	}
}
