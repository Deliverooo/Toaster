#include "toast_render/render_context.hpp"

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::render
{
	auto render_context_create(RenderContext *p_out_render_ctx, const RenderContextDesc &p_desc) -> void
	{
		gpu::GPUContextDesc gpu_context_desc{};
		gpu_context_desc.enableDebugInfo                 = true;
		gpu_context_desc.maxConcurrentSwapchainWorkloads = p_desc.maxFramesInFlight;
		gpu_context_desc.usingSwapchain                  = true;
		gpu::initGPUContext(gpu_context_desc);

		gpu::frame::initFrameContext({p_desc.maxFramesInFlight});
		gpu::alloc::initAllocationContext();
		gpu::upload::initUploadContext({});

		p_out_render_ctx->resourceHeap = gpu::createResourceDescriptorHeap({p_desc.maxBufferDescriptors, p_desc.maxTextureDescriptors});
		p_out_render_ctx->samplerHeap  = gpu::createSamplerDescriptorHeap({p_desc.maxSamplerDescriptors});
	}

	auto render_context_destroy(RenderContext *p_render_ctx) -> void
	{
		gpu::upload::shutdownUploadContext();
		gpu::alloc::shutdownAllocationContext();
		gpu::frame::shutdownFrameContext();

		// Must happen after shutting down the frame context because some deferred deletions may depend on the descriptor heaps
		gpu::destroySamplerDescriptorHeap(p_render_ctx->samplerHeap);
		gpu::destroyResourceDescriptorHeap(p_render_ctx->resourceHeap);

		p_render_ctx->samplerHeap  = nullptr;
		p_render_ctx->resourceHeap = nullptr;

		gpu::shutdownGPUContext();
	}
}
