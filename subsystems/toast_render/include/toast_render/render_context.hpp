#pragma once

#include "toast_render.hpp"
#include "toast_gpu/fwd.hpp"

namespace toaster::gpu
{
	struct GPUContext;
	struct FrameContext;
	struct AllocationContext;
	struct UploadContext;
}

namespace toaster::rnd
{
	struct TST_RENDER_API RenderContext
	{
		gpu::GPUContext *       gpuCtx;
		gpu::FrameContext *     frameCtx;
		gpu::AllocationContext *allocCtx;
		gpu::UploadContext *    uploadCtx;

		gpu::ResourceDescriptorHeapHandle resourceHeap;
		gpu::SamplerDescriptorHeapHandle  samplerHeap;
	};

	struct TST_RENDER_API RenderContextDesc
	{
		uint32 maxFramesInFlight{3u};
		uint32 maxBufferDescriptors{128u};
		uint32 maxTextureDescriptors{1028u};
		uint32 maxSamplerDescriptors{32u};
		bool   enableDebugInfo{true};
	};

	auto TST_RENDER_API render_context_create(RenderContext *p_out_render_ctx, const RenderContextDesc &p_desc) -> void;
	auto TST_RENDER_API render_context_destroy(RenderContext *p_render_ctx) -> void;
}
