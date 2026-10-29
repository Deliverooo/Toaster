#include "toast_render/render_context.hpp"

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/api.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster::rnd
{
	auto render_context_create(RenderContext *p_out_render_ctx, const RenderContextDesc &p_desc) -> void
	{
		gpu::GPUContextDesc gpu_context_desc{};
		gpu_context_desc.enableDebugInfo                 = p_desc.enableDebugInfo;
		gpu_context_desc.maxConcurrentSwapchainWorkloads = p_desc.maxFramesInFlight;
		gpu_context_desc.usingSwapchain                  = true;
		gpu::gpu_ctx_create(gpu_context_desc);

		p_out_render_ctx->gpuCtx    = gpu::gpu_ctx_create(gpu_context_desc);
		p_out_render_ctx->frameCtx  = gpu::frame_ctx_create(p_out_render_ctx->gpuCtx, {p_desc.maxFramesInFlight});
		p_out_render_ctx->allocCtx  = gpu::allocation_ctx_create(p_out_render_ctx->gpuCtx);
		p_out_render_ctx->uploadCtx = gpu::upload_ctx_create(p_out_render_ctx->gpuCtx, p_out_render_ctx->allocCtx, {});

		p_out_render_ctx->resourceHeap = gpu::resource_descriptor_heap_create(p_out_render_ctx->gpuCtx, {p_desc.maxBufferDescriptors, p_desc.maxTextureDescriptors});
		p_out_render_ctx->samplerHeap  = gpu::sampler_descriptor_heap_create(p_out_render_ctx->gpuCtx, {p_desc.maxSamplerDescriptors});

		p_out_render_ctx->maxFramesInFlight = p_desc.maxFramesInFlight;
	}

	auto render_context_destroy(RenderContext *p_render_ctx) -> void
	{
		gpu::upload_ctx_destroy(p_render_ctx->uploadCtx);
		gpu::allocation_ctx_destroy(p_render_ctx->allocCtx);
		gpu::frame_ctx_destroy(p_render_ctx->frameCtx);

		// Must happen after shutting down the frame context because some deferred deletions may depend on the descriptor heaps
		gpu::sampler_descriptor_heap_destroy(p_render_ctx->gpuCtx, p_render_ctx->samplerHeap);
		gpu::resource_descriptor_heap_destroy(p_render_ctx->gpuCtx, p_render_ctx->resourceHeap);

		p_render_ctx->samplerHeap  = nullptr;
		p_render_ctx->resourceHeap = nullptr;

		gpu::gpu_ctx_destroy(p_render_ctx->gpuCtx);

		p_render_ctx->gpuCtx    = nullptr;
		p_render_ctx->frameCtx  = nullptr;
		p_render_ctx->allocCtx  = nullptr;
		p_render_ctx->uploadCtx = nullptr;
	}
}
