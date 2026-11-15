#include <toast_gpu/api.hpp>
#include <toast_os/entry_points.hpp>

#include "toast_gpu/allocation.hpp"
#include "toast_gpu/upload.hpp"

#include <equirectangular_to_cubemap.comp.h>
#include <diffuse_irradiance_convolution.comp.h>

using namespace toaster;

enum class EShaderType : uint8
{
	eEquirectangularToCubemap     = 0u,
	eDiffuseIrradianceConvolution = 1u,
	// eSpecularIrradianceConvolution = 2u,
	eMax = 2u
};

struct ShaderLibrary
{
	TST_EARRAY(gpu::ShaderHandle, EShaderType) shaders;
};

// Because I am compiling the shaders into inline headers, I can compute the descs at compile time as well
consteval auto _get_shader_descs() -> TST_EARRAY(gpu::ShaderDesc, EShaderType)
{
	TST_EARRAY(gpu::ShaderDesc, EShaderType) out_descs;

	gpu::ShaderDesc &equirectangular_to_cubemap_desc{out_descs[EShaderType::eEquirectangularToCubemap]};
	equirectangular_to_cubemap_desc.entryPointName = "main";
	equirectangular_to_cubemap_desc.code           = c_equirectangular_to_cubemap_comp_bytecode;
	equirectangular_to_cubemap_desc.codeSizeWords  = sizeof(c_equirectangular_to_cubemap_comp_bytecode) / sizeof(uint32);
	equirectangular_to_cubemap_desc.nextStage      = gpu::EShaderStageFlagBits::eNone;
	equirectangular_to_cubemap_desc.stage          = gpu::EShaderStageFlagBits::eCompute;
	equirectangular_to_cubemap_desc.noTaskShader   = true;

	gpu::ShaderDesc &diffuse_irradiance_convolution_desc{out_descs[EShaderType::eDiffuseIrradianceConvolution]};
	diffuse_irradiance_convolution_desc.entryPointName = "main";
	diffuse_irradiance_convolution_desc.code           = c_diffuse_irradiance_convolution_comp_bytecode;
	diffuse_irradiance_convolution_desc.codeSizeWords  = sizeof(c_diffuse_irradiance_convolution_comp_bytecode) / sizeof(uint32);
	diffuse_irradiance_convolution_desc.nextStage      = gpu::EShaderStageFlagBits::eNone;
	diffuse_irradiance_convolution_desc.stage          = gpu::EShaderStageFlagBits::eCompute;
	diffuse_irradiance_convolution_desc.noTaskShader   = true;

	return out_descs;
}

inline auto shader_library_create(ShaderLibrary &p_out_shader_lib, gpu::GPUContext *p_gpu_ctx) -> void
{
	constexpr auto shader_descs{_get_shader_descs()};
	gpu::shader_create_n(p_gpu_ctx, p_out_shader_lib.shaders, shader_descs, false);
}

inline auto shader_library_destroy(ShaderLibrary &p_shader_lib, gpu::GPUContext *p_gpu_ctx) -> void
{
	gpu::shader_destroy_n(p_gpu_ctx, p_shader_lib.shaders);
}

struct TSTB
{
	gpu::GPUContext *                 gpuCtx{nullptr};
	gpu::AllocationContext *          allocCtx{nullptr};
	gpu::UploadContext *              uploadCtx{nullptr};
	gpu::ResourceDescriptorHeapHandle resourceHeap{nullptr};
	gpu::SamplerDescriptorHeapHandle  samplerHeap{nullptr};

	ShaderLibrary shaderLib;
};

TST_OS_WINMAIN()
{
	Allocator allocator{};
	allocator_create(allocator);

	TSTB tstb{};

	// Create the GPU contexts
	gpu::GPUContextDesc gpu_ctx_desc{};
	gpu_ctx_desc.enableDebugInfo = false;
	gpu_ctx_desc.usingSwapchain  = false;
	tstb.gpuCtx                  = gpu::gpu_ctx_create(gpu_ctx_desc);
	tstb.allocCtx                = gpu::allocation_ctx_create();
	tstb.uploadCtx               = gpu::upload_ctx_create(tstb.gpuCtx, tstb.allocCtx, gpu::UploadContextDesc{.maxAllocationCommandLists = 2u});

	shader_library_create(tstb.shaderLib, tstb.gpuCtx);

	tstb.resourceHeap = gpu::resource_descriptor_heap_create(tstb.gpuCtx, {1u, 16u});
	tstb.samplerHeap  = gpu::sampler_descriptor_heap_create(tstb.gpuCtx, {4u});

	gpu::SemaphoreHandle        graphics_semaphore{gpu::semaphore_create(tstb.gpuCtx, 0ull)};
	gpu::CommandAllocatorHandle command_allocator{gpu::cmd_allocator_create(tstb.gpuCtx, gpu::EQueueType::eGraphics)};
	gpu::CommandListHandle      command_list{gpu::cmd_list_allocate(tstb.gpuCtx, command_allocator)};

	{
		// Actually execute TSTB stuff...
	}

	gpu::cmd_allocator_destroy(tstb.gpuCtx, command_allocator);
	gpu::semaphore_destroy(tstb.gpuCtx, graphics_semaphore);

	gpu::sampler_descriptor_heap_destroy(tstb.gpuCtx, tstb.samplerHeap);
	gpu::resource_descriptor_heap_destroy(tstb.gpuCtx, tstb.resourceHeap);

	shader_library_destroy(tstb.shaderLib, tstb.gpuCtx);

	gpu::upload_ctx_destroy(tstb.uploadCtx);
	gpu::allocation_ctx_destroy(tstb.allocCtx);
	gpu::gpu_ctx_destroy(tstb.gpuCtx);

	allocator_destroy(allocator);

	return 0;
}
