#include "toast_render/render_context.hpp"

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	RenderContext::RenderContext()
	{
		gpu::GPUContextDesc gpu_context_desc{};
		gpu_context_desc.enableDebugInfo                 = true;
		gpu_context_desc.maxConcurrentSwapchainWorkloads = 3u;
		gpu_context_desc.usingSwapchain                  = true;
		gpu::initGPUContext(gpu_context_desc);

		gpu::frame::initFrameContext({3u});
		gpu::alloc::initAllocationContext();
		gpu::upload::initUploadContext({});

		m_resourceHeap = gpu::createResourceDescriptorHeap({16u, 64u});
		m_samplerHeap  = gpu::createSamplerDescriptorHeap({16u});
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
