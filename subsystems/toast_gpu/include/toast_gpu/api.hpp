#pragma once

#include <optional>
#include <vector>

#include "gpu_enums.hpp"
#include "toast_gpu.hpp"
#include "toast_math/math_vector.hpp"

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

	using DeviceAddress = uintptr;

	#pragma region descriptor heap

	struct TST_GPU_API ResourceDescriptorHeapDesc
	{
		// these define the maximum number of descriptors that can allocated of the specified type
		uint32 maxBufferDescriptors{32u};
		uint32 maxImageDescriptors{32u};
	};

	struct TST_GPU_API SamplerDescriptorHeapDesc
	{
		uint32 maxSamplerDescriptors{8u}; // You probably don't need that many samplers (Don't have one per image!)
	};

	#pragma endregion

	#pragma region semaphore

	struct TST_GPU_API SemaphoreSubmitInfo
	{
		SemaphoreHandle semaphore;
		uint64          value{0u};
	};

	#pragma endregion

	#pragma region buffer

	enum class EMemoryType : uint8
	{
		eDeviceLocal, eHostVisibleCoherent
	};

	enum class EBufferUsageFlagBits : uint8
	{
		eNone           = 0u,
		eTransferSrc    = TST_BIT(0u),
		eTransferDst    = TST_BIT(1u),
		eVertexBuffer   = TST_BIT(2u),
		eIndexBuffer    = TST_BIT(3u),
		eIndirectBuffer = TST_BIT(4u),
		eUniformBuffer  = TST_BIT(5u),
		eStorageBuffer  = TST_BIT(6u)
	};

	TST_SPECIALISE_FLAGS(EBufferUsageFlagBits, EBufferUsageFlags);

	struct TST_GPU_API BufferDesc
	{
		uint64            size{0u};
		EBufferUsageFlags usage{EBufferUsageFlagBits::eNone};
		EMemoryType       memoryType{EMemoryType::eDeviceLocal};
	};

	#pragma endregion

	#pragma region texture

	// Determines the vulkan image type as well as the image view type if applicable
	enum class ETextureType : uint8
	{
		e1D,
		e2D,
		e3D,
		eCube
	};

	// I don't know why you would ever need 64 samples, but I just copied this from the vulkan spec, so...
	enum class ESampleCount : uint8
	{
		e1,
		e2,
		e4,
		e8,
		e16,
		e32,
		e64
	};

	enum class ETextureUsageFlagBits : uint8
	{
		eNone                   = 0u,
		eTransferSrc            = TST_BIT(0u),
		eTransferDst            = TST_BIT(1u),
		eSampled                = TST_BIT(2u),
		eStorage                = TST_BIT(3u),
		eColourAttachment       = TST_BIT(4u),
		eDepthStencilAttachment = TST_BIT(5u),
		eTransient              = TST_BIT(6u)
	};

	TST_SPECIALISE_FLAGS(ETextureUsageFlagBits, ETextureUsageFlags);

	struct TST_GPU_API TextureDesc
	{
		tsm::uint3         extent{0u};
		uint32             mipCount{1u};
		uint32             layerCount{1u};
		ETextureType       type{ETextureType::e2D};
		ESampleCount       sampleCount{ESampleCount::e1};
		EFormat            format{EFormat::eUndefined};
		ETextureUsageFlags usage{ETextureUsageFlagBits::eNone};
	};

	#pragma endregion

	#pragma region sampler

	enum class EFilter : uint8
	{
		eNearest, eLinear
	};

	enum class ESamplerMipmapMode : uint8
	{
		eNearest, eLinear
	};

	enum class ESamplerAddressMode :uint8
	{
		eRepeat,
		eMirroredRepeat,
		eClampToEdge,
		eClampToBorder
	};

	struct TST_GPU_API SamplerDesc
	{
		EFilter             minFilter{EFilter::eLinear};
		EFilter             magFilter{EFilter::eLinear};
		ESamplerMipmapMode  mipmapMode{ESamplerMipmapMode::eLinear};
		ESamplerAddressMode addressModeU{ESamplerAddressMode::eRepeat};
		ESamplerAddressMode addressModeV{ESamplerAddressMode::eRepeat};
		ESamplerAddressMode addressModeW{ESamplerAddressMode::eRepeat};
	};

	#pragma endregion

	#pragma region rendering

	// Just all the possible typical permutations of load and store ops
	enum class EAttachmentUsageOP : uint8
	{
		// This one is most common
		eClearStore,
		eLoadStore,
		eNoneStore,
		eDontCareStore,
		eClearNone,
		eLoadNone,
		eNoneNone,
		eDontCareNone,
		eClearDontCare,
		eLoadDontCare,
		eNoneDontCare,
		eDontCareDontCare
	};

	enum class EAttachmentResolveMode : uint8
	{
		// None for no resolve operation
		eNone,
		// Idk
		eZero,
		// Normal MSAA
		eAverage,
		// Depth stencil MSAA
		eMin,
		eMax
	};

	// Copies of the vulkan structs of the same name
	struct TST_GPU_API ClearColourValue
	{
		constexpr ClearColourValue() = default;

		constexpr ClearColourValue(float32 p_r, float32 p_g, float32 p_b, float32 p_a) : _float(p_r, p_g, p_b, p_a)
		{
		}

		constexpr ClearColourValue(int32 p_r, int32 p_g, int32 p_b, int32 p_a) : _int(p_r, p_g, p_b, p_a)
		{
		}

		constexpr ClearColourValue(uint32 p_r, uint32 p_g, uint32 p_b, uint32 p_a) : _uint(p_r, p_g, p_b, p_a)
		{
		}

		union
		{
			float32 _float[4u];
			int32   _int[4u];
			uint32  _uint[4u];
		};
	};

	struct TST_GPU_API ClearDepthStencilValue
	{
		constexpr ClearDepthStencilValue() = default;

		constexpr ClearDepthStencilValue(float32 p_depth, uint32 p_stencil) : depth(p_depth), stencil(p_stencil)
		{
		}

		float32 depth{1.0f};
		uint32  stencil{0u};
	};

	union ClearValue
	{
		constexpr ClearValue(const ClearColourValue &p_colour_value = {}) : colour(p_colour_value)
		{
		}

		constexpr ClearValue(const ClearDepthStencilValue &p_clear_depth_stencil_value = {}) : depthStencil(p_clear_depth_stencil_value)
		{
		}

		ClearColourValue       colour;
		ClearDepthStencilValue depthStencil;
	};

	struct TST_GPU_API RenderingAttachmentInfo
	{
		ClearValue clearValue{ClearColourValue{}};

		TextureHandle renderTarget{nullptr};
		TextureHandle resolveTarget{nullptr};

		EAttachmentUsageOP     usageOp{EAttachmentUsageOP::eClearStore};
		EAttachmentResolveMode resolveMode{EAttachmentResolveMode::eNone}; // Only use if using multisampling
	};

	struct TST_GPU_API RenderingInfo
	{
		tsm::Rect renderArea{};

		std::vector<RenderingAttachmentInfo>   colourAttachments;
		std::optional<RenderingAttachmentInfo> depthAttachment{std::nullopt};
		std::optional<RenderingAttachmentInfo> stencilAttachment{std::nullopt};
	};

	enum class EPrimitiveTopology : uint8
	{
		ePointList,
		eLineList,
		eLineStrip,
		eTriangleList,
		eTriangleStrip,
		eTriangleFan,
		eLineListWithAdjacency,
		eLineStripWithAdjacency,
		eTriangleListWithAdjacency,
		eTriangleStripWithAdjacency,
		ePatchList
	};

	enum class EPolygonMode : uint8
	{
		eFill, eLine, ePoint
	};

	enum class ECullMode : uint8
	{
		eNone,
		eFront,
		eBack,
		eFrontAndBack
	};

	enum class EFrontFace : uint8
	{
		eCW, eCCW
	};

	enum class ECompareOp : uint8
	{
		eNever,
		eLess,
		eEqual,
		eLessOrEqual,
		eGreater,
		eNotEqual,
		eGreaterOrEqual,
		eAlways
	};

	#pragma endregion

	#pragma region shader

	enum class EShaderStageFlagBits : uint8
	{
		eNone        = 0u,
		eVertex      = TST_BIT(0u),
		ePixel       = TST_BIT(1u),
		eCompute     = TST_BIT(2u),
		eGeometry    = TST_BIT(3u),
		eTessControl = TST_BIT(4u),
		eTessEval    = TST_BIT(5u),
		eTask        = TST_BIT(6u),
		eMesh        = TST_BIT(7u)
	};

	TST_SPECIALISE_FLAGS(EShaderStageFlagBits, EShaderStageFlags);

	struct TST_GPU_API ShaderDesc
	{
		CString entryPointName{"main"};

		const uint32 *code{nullptr};
		uint64        codeSizeWords{0u};

		EShaderStageFlagBits stage{EShaderStageFlagBits::eNone};
		EShaderStageFlags    nextStage{EShaderStageFlagBits::eNone}; // The bitset of possible next stages
	};

	#pragma endregion

	struct TST_GPU_API CommandListInheritanceInfo
	{
		ResourceDescriptorHeapHandle resourceHeap{nullptr};
		SamplerDescriptorHeapHandle  samplerHeap{nullptr};

		std::vector<EFormat> colourAttachmentFormats;
		EFormat              depthAttachmentFormat{EFormat::eUndefined};
		EFormat              stencilAttachmentFormat{EFormat::eUndefined};

		ESampleCount samples{ESampleCount::e1};
	};

	enum class EQueueType : uint8
	{
		eGraphics, eCompute, eTransfer
	};

	struct TST_GPU_API GPUContextDesc
	{
		bool enableDebugInfo{true}; // Translates directly to whether validation layers are enabled.
		bool usingSwapchain{true};  // If true, the GPUContext will use the required instance and device extensions

		uint32 maxConcurrentSwapchainWorkloads{3u}; // Basically another name for the frame index. I don't want to mention the word 'frame' because there is <frame.hpp>
	};

	auto TST_GPU_API initGPUContext(const GPUContextDesc &p_desc) -> void;
	auto TST_GPU_API shutdownGPUContext() -> void;

	auto TST_GPU_API waitIdle() -> void;
	auto TST_GPU_API waitQueueIdle(EQueueType p_queue_type) -> void;

	#pragma region command list

	// If there is a command list in the free pool, returns that. Else returns a new one
	[[nodiscard]] auto TST_GPU_API getOrCreateCommandList(EQueueType p_queue_type, bool p_secondary = false) -> CommandListHandle;

	auto TST_GPU_API openCommandList(CommandListHandle p_command_list, const CommandListInheritanceInfo *p_inheritance_info = nullptr) -> void;
	auto TST_GPU_API closeCommandList(CommandListHandle p_command_list) -> void;

	// Don't worry, this doesn't actually 'free' the command list. It just returns it to the pool, so when getOrCreateCommandList is called, it is able to be returned
	// instead of having to allocate a new one.
	auto TST_GPU_API freeCommandList(CommandListHandle p_command_list) -> void;

	// Resets the command list. Check out this image for when it is appropriate to call this: https://docs.vulkan.org/spec/latest/_images/commandbuffer_lifecycle.svg
	auto TST_GPU_API resetCommandList(CommandListHandle p_command_list) -> void; // Secretly resets the command pool because it is ¿better?

	// After submitting a command list, remember to reset it so it can be recycled and used again
	// Secretly performs a secret operation alongside submission...
	// Ok, when creating a texture, it will obviously be in the undefined layout and will need an image memory barrier to be brought to life.
	// To handle this, as soon as a texture is created, it is added to a 'pending texture layout transition' queue.
	// But to execute a layout transition, you need a command buffer, so whenever a command buffer is submitted, it flushes the queue using a
	// temporary transient buffer to execute the transitions. Importantly, the transient buffer is placed before any of the other command lists in the submit info,
	// this allows all of the image memory barriers to clear before any commands that could possibly reference them to be executed.
	// If you are a Vulkan normie, you may be wondering how this works with copy operations or shader reads, as the image should be in a specific layout to perform those
	// kinds of operations. Well, the Vulkan spec introduced an extension, "vk_EXT_unified_image_layouts" which allows an image to use the same layout for every operation
	// (Except presentation). That layout is vk::ImageLayout::eGeneral. Apparently it automatically optimises for these scenarios and performs the same as it would
	// by using standard layout transitions. Look at ts: https://www.khronos.org/blog/so-long-image-layouts-simplifying-vulkan-synchronisation
	// I got this idea of batching the image layouts along with other parts of this new api from LuminaEngine: -> https://github.com/MrDrElliot/LuminaEngine
	// Thank you
	auto TST_GPU_API submit(EQueueType                                 p_queue_type, InitialiserList<const CommandListHandle> p_command_lists,
							InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos,
							InitialiserList<const SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void;

	auto TST_GPU_API executeCommandLists(CommandListHandle p_primary_command_list, InitialiserList<const CommandListHandle> p_secondary_command_lists) -> void;

	// Copy commands
	auto TST_GPU_API copyBuffer(CommandListHandle p_command_list, BufferHandle p_src_buffer, BufferHandle p_dst_buffer, uint64 p_size, uint64 p_src_offset = 0u,
								uint64            p_dst_offset                                                                                             = 0u) -> void;

	// If the texture's layout is undefined, this will insert a memory barrier
	auto TST_GPU_API copyBufferToTexture(CommandListHandle p_command_list, BufferHandle p_src_buffer, TextureHandle p_dst_texture, uint64 p_src_offset = 0u,
										 uint32            p_mip_level = 0u, uint32 p_base_layer = 0u, uint32 p_layer_count = 1u, tsm::uint3 p_extent = {}) -> void;

	auto TST_GPU_API beginRendering(CommandListHandle p_command_list, const RenderingInfo &p_rendering_info) -> void;
	auto TST_GPU_API endRendering(CommandListHandle p_command_list) -> void;

	// Literally the better version of push constants introduced alongside descriptor heaps
	auto TST_GPU_API pushData(CommandListHandle p_command_list, const void *p_data, uint64 p_size, uint32 p_offset = 0u) -> void;

	template<typename TData>
	auto TST_GPU_API pushData(CommandListHandle p_command_list, const TData &p_data, uint32 p_offset = 0u) -> void
	{
		pushData(p_command_list, &p_data, sizeof(TData), p_offset);
	}

	auto TST_GPU_API bindShaders(CommandListHandle p_command_list, InitialiserList<const ShaderHandle> p_shaders) -> void;

	auto TST_GPU_API bindResourceHeap(CommandListHandle p_command_list, ResourceDescriptorHeapHandle p_resource_heap) -> void;
	auto TST_GPU_API bindSamplerHeap(CommandListHandle p_command_list, SamplerDescriptorHeapHandle p_sampler_heap) -> void;

	auto TST_GPU_API bindIndexBuffer(CommandListHandle p_command_list, BufferHandle p_index_buffer) -> void;

	auto TST_GPU_API setPrimitiveTopology(CommandListHandle p_command_list, EPrimitiveTopology p_primitive_topology) -> void;
	auto TST_GPU_API setPrimitiveRestart(CommandListHandle p_command_list, bool p_enable, uint32 p_index = UINT32_MAX) -> void; // This should be false 99% of the time

	auto TST_GPU_API setViewport(CommandListHandle p_command_list, const tsm::Viewport &p_viewport) -> void;
	auto TST_GPU_API setScissor(CommandListHandle p_command_list, const tsm::Rect &p_scissor_rect) -> void;

	auto TST_GPU_API setRasterizerDiscardEnable(CommandListHandle p_command_list, bool p_enable) -> void;
	auto TST_GPU_API setPolygonMode(CommandListHandle p_command_list, EPolygonMode p_polygon_mode) -> void;
	auto TST_GPU_API setCullMode(CommandListHandle p_command_list, ECullMode p_cull_mode) -> void;
	auto TST_GPU_API setFrontFace(CommandListHandle p_command_list, EFrontFace p_front_face) -> void;
	auto TST_GPU_API setDepthBias(CommandListHandle p_command_list, bool p_enable, float32 p_constant_factor = 0.0f, float32 p_clamp = 0.0f,
								  float32           p_slope_factor                                           = 0.0f) -> void;
	auto TST_GPU_API setLineWidth(CommandListHandle p_command_list, float32 p_line_width) -> void;

	auto TST_GPU_API setRasterizationSamples(CommandListHandle p_command_list, ESampleCount p_sample_count) -> void;

	auto TST_GPU_API setDepthState(CommandListHandle p_command_list, bool p_test_enable, bool p_write_enable = true, bool p_clamp_enable = false,
								   ECompareOp        p_compare_op                                            = ECompareOp::eLessOrEqual) -> void;
	auto TST_GPU_API setStencilState(CommandListHandle p_command_list, bool p_test_enable) -> void;

	struct TST_GPU_API DrawIndirectCommand
	{
		uint32 vertexCount{0u};
		uint32 instanceCount{1u};
		uint32 firstVertex{0u};
		uint32 firstInstance{0u};
	};

	struct TST_GPU_API DrawIndexedIndirectCommand
	{
		uint32 indexCount{0u};
		uint32 instanceCount{1u};
		uint32 firstIndex{0u};
		int32  vertexOffset{0};
		uint32 firstInstance{0u};
	};

	auto TST_GPU_API draw(CommandListHandle p_command_list, uint32 p_vertex_count, uint32 p_instance_count, uint32 p_first_vertex = 0u,
						  uint32            p_first_instance                                                                      = 0u) -> void;
	auto TST_GPU_API drawIndexed(CommandListHandle p_command_list, uint32 p_index_count, uint32 p_instance_count, uint32 p_first_index = 0u, int32 p_vertex_offset = 0,
								 uint32            p_first_instance                                                                    = 0u) -> void;
	auto TST_GPU_API drawIndirect(CommandListHandle p_command_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
								  uint32            p_stride = sizeof(DrawIndirectCommand)) -> void;
	auto TST_GPU_API drawIndexedIndirect(CommandListHandle p_command_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
										 uint32            p_stride = sizeof(DrawIndexedIndirectCommand)) -> void;

	#pragma endregion

	#pragma region descriptor heaps

	[[nodiscard]] auto TST_GPU_API createResourceDescriptorHeap(const ResourceDescriptorHeapDesc &p_desc) -> ResourceDescriptorHeapHandle;
	[[nodiscard]] auto TST_GPU_API createSamplerDescriptorHeap(const SamplerDescriptorHeapDesc &p_desc) -> SamplerDescriptorHeapHandle;

	auto TST_GPU_API destroyResourceDescriptorHeap(ResourceDescriptorHeapHandle p_resource_heap) -> void;
	auto TST_GPU_API destroySamplerDescriptorHeap(SamplerDescriptorHeapHandle p_sampler_heap) -> void;

	[[nodiscard]] auto TST_GPU_API allocBufferHeapSlot(ResourceDescriptorHeapHandle p_resource_heap) -> uint32;
	[[nodiscard]] auto TST_GPU_API allocTextureHeapSlot(ResourceDescriptorHeapHandle p_resource_heap) -> uint32;
	[[nodiscard]] auto TST_GPU_API allocSamplerHeapSlot(SamplerDescriptorHeapHandle p_sampler_heap) -> uint32;

	auto TST_GPU_API freeBufferHeapSlot(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void;
	auto TST_GPU_API freeTextureHeapSlot(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void;
	auto TST_GPU_API freeSamplerHeapSlot(SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot) -> void;

	auto TST_GPU_API writeBufferDescriptor(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, BufferHandle p_buffer) -> void;
	auto TST_GPU_API writeTextureDescriptor(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, TextureHandle p_texture, bool p_storage,
											uint32                       p_mip = UINT32_MAX) -> void;
	auto TST_GPU_API writeSamplerDescriptor(SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot, SamplerHandle p_sampler) -> void;

	#pragma endregion

	#pragma region buffer

	[[nodiscard]] auto TST_GPU_API createBuffer(const BufferDesc &p_desc) -> BufferHandle;
	auto TST_GPU_API               destroyBuffer(BufferHandle p_buffer) -> void;

	auto TST_GPU_API               writeBufferData(BufferHandle p_buffer, const void *p_data, uint64 p_size, uint64 p_offset = 0u) -> void; // If host-visible
	[[nodiscard]] auto TST_GPU_API getBufferMappedData(BufferHandle p_buffer) -> void *;

	[[nodiscard]] auto TST_GPU_API getBufferAddress(BufferHandle p_buffer) -> DeviceAddress;

	#pragma endregion

	// I am NOT getting something like GLFW involved at this level. Just pass in the raw HWND.
	// For GLFW users: #define GLFW_EXPOSE_NATIVE_WIN32
	//				   #include <GLFW/glfw3native.h>
	//				   ...
	//				   namespace tst = toaster;
	//				   tst::gpu::SurfaceHandle surface{tst::gpu::createSurface(glfwGetWin32Window(the_window_that_you_have_probably_already_created_by_this_point))};
	// I thought I could be smart and cache the surface caps for the vk::SurfaceKHR. Yeh, apparently that is actually not safe at all because the surface is a way for
	// Vulkan to interface with your window. It is not in any way tied to one specific monitor.
	// So if Hypothetically, you were to drag the window from one 4k 670Hz monitor over to another 720p 60Hz monitor. The physical surface capabilities which
	// the monitor supports would obviously change. This means that caching them is out of the question... :(
	[[nodiscard]] auto TST_GPU_API createSurface(void *p_hwnd) -> SurfaceHandle;
	auto TST_GPU_API               destroySurface(SurfaceHandle p_surface) -> void;

	#pragma region synchronisation

	// Secretly a timeline semaphore...
	// After finally overcoming my fear of timeline semaphores, I awakened to the fact that they are objectively better than fences and binary semaphores altogether.
	// (Again, except for CERTAIN swapchain operations, which mandate the usage of binary semaphores). This realisation came to me while using DirectX 12.
	// If you are familiar, you will know that while swapchain usage is heavily simplified, you will still have to manage concurrent gpu work jobs over multiple frames
	// using ID3D12Fences. Little to my knowledge at the time, these were practically identical to Vulkan's timeline semaphores. So when I came back to Vulkan after my
	// DirectX 12 side quest, I finally understood where all the hype around timeline semaphores came from.
	// Now I am a proud advocate against the use of vk::Fences in favour of timeline semaphores
	[[nodiscard]] auto TST_GPU_API createSemaphore(uint64 p_initial_value = 0u) -> SemaphoreHandle;
	auto TST_GPU_API               destroySemaphore(SemaphoreHandle p_semaphore) -> void;
	auto TST_GPU_API               waitSemaphores(InitialiserList<const SemaphoreHandle> p_semaphores, InitialiserList<const uint64> p_values) -> void;
	auto TST_GPU_API               getSemaphoreValue(SemaphoreHandle p_semaphore) -> uint64;

	#pragma endregion

	#pragma region texture + sampler

	[[nodiscard]] auto TST_GPU_API createTexture(const TextureDesc &p_desc) -> TextureHandle;
	auto TST_GPU_API               destroyTexture(TextureHandle p_texture) -> void;

	[[nodiscard]] auto TST_GPU_API getTextureDesc(TextureHandle p_texture) -> const TextureDesc &;

	[[nodiscard]] auto TST_GPU_API createSampler(const SamplerDesc &p_desc) -> SamplerHandle;
	auto TST_GPU_API               destroySampler(SamplerHandle p_sampler) -> void;

	#pragma endregion

	#pragma region swapchain

	[[nodiscard]] auto TST_GPU_API createSwapchain(SurfaceHandle p_surface, tsm::uint2 p_initial_extent) -> SwapchainHandle;
	auto TST_GPU_API               destroySwapchain(SwapchainHandle p_swapchain) -> void;

	// Returns true if successful, false otherwise. Use this to skip rendering if unsuccessful and retry
	auto TST_GPU_API resizeSwapchain(SwapchainHandle p_swapchain, tsm::uint2 p_extent) -> bool; // Resize is identical to 'Recreate'...

	[[nodiscard]] auto TST_GPU_API acquireNextImage(SwapchainHandle p_swapchain) -> TextureHandle; // If the image was unable to be acquired. Returns a null handle

	// Fancy way of saying present src to general
	auto TST_GPU_API insertPreRenderSwapchainResourceBarrier(CommandListHandle p_command_list, TextureHandle p_attachment_texture) -> void;

	// Returns true if successful, false if the present operation was unsuccessful. Recreate if false
	auto TST_GPU_API submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list, const SemaphoreSubmitInfo &p_signal_semaphore_info) -> bool;
	auto TST_GPU_API submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list, const SemaphoreSubmitInfo &p_signal_semaphore_info,
									  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos) -> bool;

	#pragma endregion

	#pragma region shader

	[[nodiscard]] auto TST_GPU_API createShader(const ShaderDesc &p_desc) -> ShaderHandle;
	auto TST_GPU_API               destroyShader(ShaderHandle p_shader) -> void;

	#pragma endregion
}
