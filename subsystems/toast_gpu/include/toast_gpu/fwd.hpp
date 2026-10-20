#pragma once

#include "toast_gpu.hpp"

namespace toaster::gpu
{
	TST_GPU_DECLARE_GPU_HANDLE(CommandList);
	TST_GPU_DECLARE_GPU_HANDLE(ResourceDescriptorHeap);
	TST_GPU_DECLARE_GPU_HANDLE(SamplerDescriptorHeap);
	TST_GPU_DECLARE_GPU_HANDLE(Semaphore);
	TST_GPU_DECLARE_GPU_HANDLE(Buffer);
	TST_GPU_DECLARE_GPU_HANDLE(Texture);
	TST_GPU_DECLARE_GPU_HANDLE(Sampler);
	TST_GPU_DECLARE_GPU_HANDLE(Surface);
	TST_GPU_DECLARE_GPU_HANDLE(Swapchain);
	TST_GPU_DECLARE_GPU_HANDLE(Shader);
}
