#pragma once

#include "toast_gpu.hpp"

namespace toaster::gpu
{
	using DeviceAddress = uintptr;
	constexpr DeviceAddress nullGpuPtr{0u};

	struct GPUContext;

	TST_DECLARE_HANDLE(CommandList);
	TST_DECLARE_HANDLE(ResourceDescriptorHeap);
	TST_DECLARE_HANDLE(SamplerDescriptorHeap);
	TST_DECLARE_HANDLE(Semaphore);
	TST_DECLARE_HANDLE(Buffer);
	TST_DECLARE_HANDLE(Texture);
	TST_DECLARE_HANDLE(Sampler);
	TST_DECLARE_HANDLE(Surface);
	TST_DECLARE_HANDLE(Swapchain);
	TST_DECLARE_HANDLE(Shader);

	struct AllocationContext;
	TST_DECLARE_HANDLE(VirtualBlock);
	TST_DECLARE_HANDLE(VirtualAllocation);

	struct UploadContext;
	TST_DECLARE_HANDLE(StateTracker);
}
