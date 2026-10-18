#pragma once

#include "toast_gpu.hpp"

namespace toaster::gpu
{
	TST_DECLARE_GPU_HANDLE(CommandList);
	TST_DECLARE_GPU_HANDLE(ResourceDescriptorHeap);
	TST_DECLARE_GPU_HANDLE(SamplerDescriptorHeap);
	TST_DECLARE_GPU_HANDLE(Semaphore);
	TST_DECLARE_GPU_HANDLE(Buffer);
	TST_DECLARE_GPU_HANDLE(Texture);
	TST_DECLARE_GPU_HANDLE(Sampler);
	TST_DECLARE_GPU_HANDLE(Surface);
	TST_DECLARE_GPU_HANDLE(Swapchain);
	TST_DECLARE_GPU_HANDLE(Shader);
}
