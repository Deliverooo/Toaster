#pragma once

#include <optional>
#include <span>

#include "fwd.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::gpu
{
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

	#pragma region synchronisation

	struct TST_GPU_API SemaphoreSubmitInfo
	{
		SemaphoreHandle semaphore;
		uint64          value{0u};
	};

	enum class EQueueType : uint8
	{
		eGraphics = 0u,
		eCompute  = 1u,
		eTransfer = 2u,
		eMax      = 3u
	};

	constexpr uint8 c_queueCount{TST_ENUM_VAL(EQueueType::eMax)};

	inline CString to_string(EQueueType p_queue_type)
	{
		switch (p_queue_type)
		{
			case EQueueType::eGraphics: return "Graphics";
			case EQueueType::eCompute: return "Compute";
			case EQueueType::eTransfer: return "Transfer";
			default: return "Unknown";
		}
	}

	enum class EAccessFlagBits : uint32
	{
		eNone                        = TST_BIT(0u),
		eIndirectCommandRead         = TST_BIT(1u),
		eIndexRead                   = TST_BIT(2u),
		eVertexAttributeRead         = TST_BIT(3u),
		eUniformRead                 = TST_BIT(4u),
		eInputAttachmentRead         = TST_BIT(5u),
		eShaderRead                  = TST_BIT(6u),
		eShaderWrite                 = TST_BIT(7u),
		eColorAttachmentRead         = TST_BIT(8u),
		eColorAttachmentWrite        = TST_BIT(9u),
		eDepthStencilAttachmentRead  = TST_BIT(10u),
		eDepthStencilAttachmentWrite = TST_BIT(11u),
		eTransferRead                = TST_BIT(12u),
		eTransferWrite               = TST_BIT(13u),
		eHostRead                    = TST_BIT(14u),
		eHostWrite                   = TST_BIT(15u),
		eMemoryRead                  = TST_BIT(16u),
		eMemoryWrite                 = TST_BIT(17u),
		eShaderSampledRead           = TST_BIT(18u),
		eShaderStorageRead           = TST_BIT(19u),
		eShaderStorageWrite          = TST_BIT(20u),
		eSamplerHeapRead             = TST_BIT(21u),
		eResourceHeapRead            = TST_BIT(22u)
	};

	TST_SPECIALISE_FLAGS(EAccessFlagBits, EAccessFlags);

	enum class EPipelineStageFlagBits : uint32
	{
		eNone                         = TST_BIT(0u),
		eTopOfPipe                    = TST_BIT(1u),
		eDrawIndirect                 = TST_BIT(2u),
		eVertexInput                  = TST_BIT(3u),
		eVertexShader                 = TST_BIT(4u),
		eTessellationControlShader    = TST_BIT(5u),
		eTessellationEvaluationShader = TST_BIT(6u),
		eGeometryShader               = TST_BIT(7u),
		eFragmentShader               = TST_BIT(8u),
		eEarlyFragmentTests           = TST_BIT(9u),
		eLateFragmentTests            = TST_BIT(10u),
		eColorAttachmentOutput        = TST_BIT(11u),
		eComputeShader                = TST_BIT(12u),
		eAllTransfer                  = TST_BIT(13u),
		eTransfer                     = TST_BIT(14u),
		eBottomOfPipe                 = TST_BIT(15u),
		eHost                         = TST_BIT(16u),
		eAllGraphics                  = TST_BIT(17u),
		eAllCommands                  = TST_BIT(18u),
		eCopy                         = TST_BIT(19u),
		eResolve                      = TST_BIT(20u),
		eBlit                         = TST_BIT(21u),
		eClear                        = TST_BIT(22u),
		eIndexInput                   = TST_BIT(23u),
		eVertexAttributeInput         = TST_BIT(24u),
		ePreRasterizationShaders      = TST_BIT(25u),
		eTaskShader                   = TST_BIT(26u),
		eMeshShader                   = TST_BIT(27u)
	};

	TST_SPECIALISE_FLAGS(EPipelineStageFlagBits, EPipelineStageFlags);

	constexpr uint64 c_wholeSize{~0ull}; // This means use the actual full size of the object

	struct TST_GPU_API BufferMemoryBarrier
	{
		BufferHandle buffer{nullptr};
		uint64       offset{0ull};
		uint64       size{c_wholeSize};

		EAccessFlags srcAccessMask{0u};
		EAccessFlags dstAccessMask{0u};

		EPipelineStageFlags srcStageMask{0u};
		EPipelineStageFlags dstStageMask{0u};

		EQueueType srcQueue{EQueueType::eGraphics};
		EQueueType dstQueue{EQueueType::eGraphics};
	};

	struct TST_GPU_API TextureMemoryBarrier
	{
		TextureHandle texture{nullptr};

		EAccessFlags srcAccessMask{0u};
		EAccessFlags dstAccessMask{0u};

		EPipelineStageFlags srcStageMask{0u};
		EPipelineStageFlags dstStageMask{0u};

		uint32 baseArrayLayer{0u};
		uint32 baseMip{0u};
		uint32 layerCount{UINT32_MAX}; // UINT32_MAX uses the one from the desc
		uint32 mipCount{UINT32_MAX};

		EQueueType srcQueue{EQueueType::eGraphics};
		EQueueType dstQueue{EQueueType::eGraphics};
	};

	enum class EFrameMarker : uint8
	{
		eSimulationStart,
		eSimulationEnd,
		eRendersubmitStart,
		eRendersubmitEnd,
		ePresentStart,
		ePresentEnd,
		eInputSample,
		eTriggerFlash,
		eOutOfBandRendersubmitStart,
		eOutOfBandRendersubmitEnd,
		eOutOfBandPresentStart,
		eOutOfBandPresentEnd
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

	enum class EFormat : uint8 // Taken directly from vulkan.hpp
	{
		eUndefined,
		eR8Unorm,
		eR8Snorm,
		eR8Uint,
		eR8Sint,
		eR8Srgb,
		eR8G8Unorm,
		eR8G8Snorm,
		eR8G8Uint,
		eR8G8Sint,
		eR8G8Srgb,
		eR8G8B8Unorm,
		eR8G8B8Snorm,
		eR8G8B8Uint,
		eR8G8B8Sint,
		eR8G8B8Srgb,
		eB8G8R8Unorm,
		eB8G8R8Snorm,
		eB8G8R8Uint,
		eB8G8R8Sint,
		eB8G8R8Srgb,
		eR8G8B8A8Unorm,
		eR8G8B8A8Snorm,
		eR8G8B8A8Uint,
		eR8G8B8A8Sint,
		eR8G8B8A8Srgb,
		eB8G8R8A8Unorm,
		eB8G8R8A8Snorm,
		eB8G8R8A8Uint,
		eB8G8R8A8Sint,
		eB8G8R8A8Srgb,
		eR16Unorm,
		eR16Snorm,
		eR16Uint,
		eR16Sint,
		eR16Sfloat,
		eR16G16Unorm,
		eR16G16Snorm,
		eR16G16Uint,
		eR16G16Sint,
		eR16G16Sfloat,
		eR16G16B16Unorm,
		eR16G16B16Snorm,
		eR16G16B16Uint,
		eR16G16B16Sint,
		eR16G16B16Sfloat,
		eR16G16B16A16Unorm,
		eR16G16B16A16Snorm,
		eR16G16B16A16Uint,
		eR16G16B16A16Sint,
		eR16G16B16A16Sfloat,
		eR32Uint,
		eR32Sint,
		eR32Sfloat,
		eR32G32Uint,
		eR32G32Sint,
		eR32G32Sfloat,
		eR32G32B32Uint,
		eR32G32B32Sint,
		eR32G32B32Sfloat,
		eR32G32B32A32Uint,
		eR32G32B32A32Sint,
		eR32G32B32A32Sfloat,
		eR64Uint,
		eR64Sint,
		eR64Sfloat,
		eR64G64Uint,
		eR64G64Sint,
		eR64G64Sfloat,
		eR64G64B64Uint,
		eR64G64B64Sint,
		eR64G64B64Sfloat,
		eR64G64B64A64Uint,
		eR64G64B64A64Sint,
		eR64G64B64A64Sfloat,
		eD16Unorm,
		eD32Sfloat,
		eS8Uint,
		eD16UnormS8Uint,
		eD24UnormS8Uint,
		eD32SfloatS8Uint
	};

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
		math::UInt3        extent{0u, 0u, 0u};
		uint32             mipCount{1u};
		uint32             layerCount{1u};
		ETextureType       type{ETextureType::e2D};
		ESampleCount       sampleCount{ESampleCount::e1};
		EFormat            format{EFormat::eUndefined};
		ETextureUsageFlags usage{ETextureUsageFlagBits::eNone};
	};

	constexpr uint32 c_allMipLevels{UINT32_MAX};

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
		float32             minLod{0.0f};
		float32             maxLod{1000.0f};
		float32             maxAnisotropy{-1.0f}; // -1	means choose the physical device's limit
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
		math::Rect renderArea{};

		Vector<RenderingAttachmentInfo>        colourAttachments;
		std::optional<RenderingAttachmentInfo> depthAttachment{std::nullopt};
		std::optional<RenderingAttachmentInfo> stencilAttachment{std::nullopt};
		bool                                   useSecondaryCommandBuffers{false};
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

	enum class EColourComponentFlagBits : uint8
	{
		eR   = TST_BIT(0u),
		eG   = TST_BIT(1u),
		eB   = TST_BIT(2u),
		eA   = TST_BIT(3u),
		eAll = eR | eG | eB | eA
	};

	TST_SPECIALISE_FLAGS(EColourComponentFlagBits, EColourComponentFlags);

	enum class EBlendFactor : uint8
	{
		eZero,
		eOne,
		eSrcColor,
		eOneMinusSrcColor,
		eDstColor,
		eOneMinusDstColor,
		eSrcAlpha,
		eOneMinusSrcAlpha,
		eDstAlpha,
		eOneMinusDstAlpha,
		eConstantColor,
		eOneMinusConstantColor,
		eConstantAlpha,
		eOneMinusConstantAlpha,
		eSrcAlphaSaturate,
		eSrc1Color,
		eOneMinusSrc1Color,
		eSrc1Alpha,
		eOneMinusSrc1Alpha
	};

	enum class EBlendOp : uint8
	{
		eAdd,
		eSubtract,
		eReverseSubtract,
		eMin,
		eMax
	};

	struct TST_GPU_API ColourBlendEquation
	{
		EBlendFactor srcColorBlendFactor{EBlendFactor::eZero};
		EBlendFactor dstColorBlendFactor{EBlendFactor::eZero};
		EBlendOp     colorBlendOp{EBlendOp::eAdd};
		EBlendFactor srcAlphaBlendFactor{EBlendFactor::eZero};
		EBlendFactor dstAlphaBlendFactor{EBlendFactor::eZero};
		EBlendOp     alphaBlendOp{EBlendOp::eAdd};
	};

	enum class EIndexType : uint8
	{
		eUint8, eUint16, eUint32
	};

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

	struct TST_GPU_API DrawMeshTasksIndirectCommand
	{
		uint32 groupCountX{0u};
		uint32 groupCountY{0u};
		uint32 groupCountZ{0u};
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

		EShaderStageFlags    nextStage{EShaderStageFlagBits::eNone}; // The bitset of possible next stages
		EShaderStageFlagBits stage{EShaderStageFlagBits::eNone};

		bool noTaskShader{false};
	};

	#pragma endregion

	#pragma region transfer command

	struct TST_GPU_API TextureSubresource
	{
		uint32 mipLevel{0u};
		uint32 baseLayer{0u};
		uint32 layerCount{1u};
	};

	struct TST_GPU_API BufferTextureCopyRegion
	{
		uint64     bufferOffset{0ull};
		math::Int3 textureOffset{0, 0, 0};
		// If any of the components in textureExtent are 0, it will use the actual extent for that dimension from the texture's internal desc
		math::UInt3        textureExtent{0u, 0u, 0u};
		TextureSubresource textureSubresource{};
	};

	#pragma endregion

	struct TST_GPU_API CommandListInheritanceInfo
	{
		ResourceDescriptorHeapHandle resourceHeap{nullptr};
		SamplerDescriptorHeapHandle  samplerHeap{nullptr};

		Vector<EFormat> colourAttachmentFormats;
		EFormat         depthAttachmentFormat{EFormat::eUndefined};
		EFormat         stencilAttachmentFormat{EFormat::eUndefined};

		ESampleCount samples{ESampleCount::e1};
	};

	struct TST_GPU_API GPUContextDesc
	{
		bool enableDebugInfo{true}; // Translates directly to whether validation layers are enabled.
		bool usingSwapchain{true};  // If true, the GPUContext will use the required instance and device extensions

		uint32 maxConcurrentSwapchainWorkloads{3u}; // Basically another name for the frame index. I don't want to mention the word 'frame' because there is <frame.hpp>
	};

	struct GPUContext;

	auto TST_GPU_API gpu_ctx_create(const GPUContextDesc &p_desc) -> GPUContext *;
	auto TST_GPU_API gpu_ctx_destroy(GPUContext *p_ctx) -> void;

	auto TST_GPU_API gpu_ctx_print_allocation_statistics(const GPUContext *p_ctx) -> void;
	auto TST_GPU_API gpu_ctx_dump_allocation_statistics_to_json(const GPUContext *p_ctx, StringView p_path) -> void;

	auto TST_GPU_API wait_idle(const GPUContext *p_ctx) -> void;
	auto TST_GPU_API wait_queue_idle(const GPUContext *p_ctx, EQueueType p_queue_type) -> void;

	#pragma region command allocator / command list

	// Command allocators are meant to be per-thread; therefore, they are not internally synchronised.
	auto TST_GPU_API cmd_allocator_create(GPUContext *p_ctx, EQueueType p_queue_type) -> CommandAllocatorHandle;
	auto TST_GPU_API cmd_allocator_destroy(GPUContext *p_ctx, CommandAllocatorHandle p_cmd_allocator) -> void;

	// Resets the command list. Check out this image for when it is appropriate to call this: https://docs.vulkan.org/spec/latest/_images/commandbuffer_lifecycle.svg
	auto TST_GPU_API cmd_allocator_reset(GPUContext *p_ctx, CommandAllocatorHandle p_cmd_allocator) -> void;

	auto TST_GPU_API cmd_list_allocate_n(GPUContext *p_ctx, CommandAllocatorHandle p_cmd_allocator, std::span<CommandListHandle> p_out_cmd_lists,
										 bool        p_secondary = false) -> void;

	// Don't worry, this doesn't actually 'free' the command list. It just returns it to the allocator's free list, so when cmd_list_create is called, it is able to be returned
	// instead of having to allocate a new one.
	auto TST_GPU_API cmd_list_free(GPUContext *p_ctx, CommandAllocatorHandle p_cmd_allocator, CommandListHandle p_cmd_list) -> void;

	// Requires p_cmd_lists to either be all primary or all secondary, dictated by p_secondary
	auto TST_GPU_API cmd_list_free_n(GPUContext *p_ctx, CommandAllocatorHandle p_cmd_allocator, InitialiserList<const CommandListHandle> p_cmd_lists,
									 bool        p_secondary) -> void;

	auto TST_GPU_API cmd_list_open(GPUContext *p_ctx, CommandListHandle p_cmd_list, const CommandListInheritanceInfo *p_inheritance_info = nullptr) -> void;
	auto TST_GPU_API cmd_list_close(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void;

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
	auto TST_GPU_API queue_submit(GPUContext *                               p_ctx, EQueueType p_queue_type, InitialiserList<const CommandListHandle> p_cmd_lists,
								  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos,
								  InitialiserList<const SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void;

	auto TST_GPU_API cmd_execute_cmd_lists(GPUContext *                             p_ctx, CommandListHandle p_primary_command_list,
										   InitialiserList<const CommandListHandle> p_secondary_command_lists) -> void;

	// Copy command
	auto TST_GPU_API cmd_copy_buffer(GPUContext *p_ctx, CommandListHandle  p_cmd_list, BufferHandle p_src_buffer, BufferHandle p_dst_buffer, uint64 p_size,
									 uint64      p_src_offset = 0u, uint64 p_dst_offset = 0u) -> void;

	auto TST_GPU_API cmd_copy_buffer_to_texture(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_src_buffer, TextureHandle p_dst_texture,
												InitialiserList<const BufferTextureCopyRegion> p_copy_regions) -> void;

	auto TST_GPU_API cmd_fill_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_dst_offset, uint64 p_size, uint32 p_data) -> void;

	auto TST_GPU_API cmd_pipeline_barrier(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const BufferMemoryBarrier> p_buffer_memory_barriers,
										  InitialiserList<const TextureMemoryBarrier> p_texture_memory_barriers) -> void;

	auto TST_GPU_API cmd_generate_mipmaps(GPUContext *p_ctx, CommandListHandle p_cmd_list, TextureHandle p_texture) -> void;

	auto TST_GPU_API cmd_begin_rendering(GPUContext *p_ctx, CommandListHandle p_cmd_list, const RenderingInfo &p_rendering_info) -> void;
	auto TST_GPU_API cmd_end_rendering(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void;

	// Literally the better version of push constants introduced alongside descriptor heaps
	auto TST_GPU_API cmd_push_data(GPUContext *p_ctx, CommandListHandle p_cmd_list, const void *p_data, uint64 p_size, uint32 p_offset = 0u) -> void;

	template<typename TData>
	auto TST_GPU_API cmd_push_data(GPUContext *p_ctx, CommandListHandle p_cmd_list, const TData &p_data, uint32 p_offset = 0u) -> void
	{
		cmd_push_data(p_ctx, p_cmd_list, &p_data, sizeof(TData), p_offset);
	}

	auto TST_GPU_API cmd_bind_shaders(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const ShaderHandle> p_shaders) -> void;

	auto TST_GPU_API cmd_bind_resource_heap(GPUContext *p_ctx, CommandListHandle p_cmd_list, ResourceDescriptorHeapHandle p_resource_heap) -> void;
	auto TST_GPU_API cmd_bind_sampler_heap(GPUContext *p_ctx, CommandListHandle p_cmd_list, SamplerDescriptorHeapHandle p_sampler_heap) -> void;

	auto TST_GPU_API cmd_bind_index_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_index_buffer, EIndexType p_index_type) -> void;

	auto TST_GPU_API cmd_set_primitive_topology(GPUContext *p_ctx, CommandListHandle p_cmd_list, EPrimitiveTopology p_primitive_topology) -> void;
	// This should be false 99% of the time
	auto TST_GPU_API cmd_set_primitive_restart(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable, uint32 p_index = UINT32_MAX) -> void;

	auto TST_GPU_API cmd_set_viewport(GPUContext *p_ctx, CommandListHandle p_cmd_list, const math::Viewport &p_viewport) -> void;
	auto TST_GPU_API cmd_set_scissor(GPUContext *p_ctx, CommandListHandle p_cmd_list, const math::Rect &p_scissor_rect) -> void;

	auto TST_GPU_API cmd_set_rasterizer_discard_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable) -> void;
	auto TST_GPU_API cmd_set_polygon_mode(GPUContext *p_ctx, CommandListHandle p_cmd_list, EPolygonMode p_polygon_mode) -> void;
	auto TST_GPU_API cmd_set_cull_mode(GPUContext *p_ctx, CommandListHandle p_cmd_list, ECullMode p_cull_mode) -> void;
	auto TST_GPU_API cmd_set_front_face(GPUContext *p_ctx, CommandListHandle p_cmd_list, EFrontFace p_front_face) -> void;
	auto TST_GPU_API cmd_set_depth_bias(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable, float32 p_constant_factor = 0.0f, float32 p_clamp = 0.0f,
										float32     p_slope_factor                                                                = 0.0f) -> void;
	auto TST_GPU_API cmd_set_line_width(GPUContext *p_ctx, CommandListHandle p_cmd_list, float32 p_line_width) -> void;

	auto TST_GPU_API cmd_set_rasterization_samples(GPUContext *p_ctx, CommandListHandle p_cmd_list, ESampleCount p_sample_count) -> void;

	auto TST_GPU_API cmd_set_depth_state(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_test_enable, bool p_write_enable = true, bool p_clamp_enable = false,
										 ECompareOp  p_compare_op                                                                 = ECompareOp::eLessOrEqual) -> void;
	auto TST_GPU_API cmd_set_stencil_state(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_test_enable) -> void;

	auto TST_GPU_API cmd_set_colour_write_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const bool32> p_enables) -> void;
	auto TST_GPU_API cmd_set_colour_write_mask(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const EColourComponentFlags> p_masks,
											   uint32      p_first_attachment_index = 0u) -> void;

	auto TST_GPU_API cmd_set_colour_blend_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const bool32> p_enables,
												 uint32      p_first_attachment_index = 0u) -> void;

	auto TST_GPU_API cmd_set_colour_blend_equation(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const ColourBlendEquation> p_blend_equations,
												   uint32      p_first_attachment_index = 0u) -> void;

	auto TST_GPU_API cmd_draw(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_vertex_count, uint32 p_instance_count, uint32 p_first_vertex = 0u,
							  uint32 p_first_instance = 0u) -> void;
	auto TST_GPU_API cmd_draw_indexed(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_index_count, uint32 p_instance_count, uint32 p_first_index = 0u,
									  int32       p_vertex_offset = 0, uint32 p_first_instance = 0u) -> void;
	auto TST_GPU_API cmd_draw_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
									   uint32      p_stride = sizeof(DrawIndirectCommand)) -> void;
	auto TST_GPU_API cmd_draw_indexed_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
											   uint32      p_stride = sizeof(DrawIndexedIndirectCommand)) -> void;

	auto TST_GPU_API cmd_draw_mesh_tasks(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_group_count_x, uint32 p_group_count_y, uint32 p_group_count_z) -> void;
	auto TST_GPU_API cmd_draw_mesh_tasks_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
												  uint32 p_stride = sizeof(DrawMeshTasksIndirectCommand)) -> void;
	auto TST_GPU_API cmd_draw_mesh_tasks_indirect_count(GPUContext * p_ctx, CommandListHandle p_cmd_list, BufferHandle p_indirect_buffer, uint64 p_indirect_buffer_offset,
														BufferHandle p_count_buffer, uint64   p_count_buffer_offset, uint32 p_max_draw_count,
														uint32       p_stride = sizeof(DrawMeshTasksIndirectCommand)) -> void;

	auto TST_GPU_API cmd_dispatch(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_work_groups_x, uint32 p_work_groups_y, uint32 p_work_groups_z) -> void;

	#pragma endregion

	#pragma region descriptor heaps

	[[nodiscard]] auto TST_GPU_API resource_descriptor_heap_create(GPUContext *p_ctx, const ResourceDescriptorHeapDesc &p_desc) -> ResourceDescriptorHeapHandle;
	[[nodiscard]] auto TST_GPU_API sampler_descriptor_heap_create(GPUContext *p_ctx, const SamplerDescriptorHeapDesc &p_desc) -> SamplerDescriptorHeapHandle;

	auto TST_GPU_API resource_descriptor_heap_destroy(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap) -> void;
	auto TST_GPU_API sampler_descriptor_heap_destroy(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap) -> void;

	[[nodiscard]] auto TST_GPU_API resource_descriptor_heap_alloc_buffer_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap) -> uint32;
	auto TST_GPU_API               resource_descriptor_heap_alloc_buffer_slots(GPUContext *      p_ctx, ResourceDescriptorHeapHandle p_resource_heap,
																			   std::span<uint32> p_out_heap_slots) -> void;
	[[nodiscard]] auto TST_GPU_API resource_descriptor_heap_alloc_texture_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap) -> uint32;
	auto TST_GPU_API               resource_descriptor_heap_alloc_texture_slots(GPUContext *      p_ctx, ResourceDescriptorHeapHandle p_resource_heap,
																				std::span<uint32> p_out_heap_slots) -> void;
	[[nodiscard]] auto TST_GPU_API sampler_descriptor_heap_alloc_sampler_slot(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap) -> uint32;
	auto TST_GPU_API               sampler_descriptor_heap_alloc_sampler_slots(GPUContext *      p_ctx, SamplerDescriptorHeapHandle p_sampler_heap,
																			   std::span<uint32> p_out_heap_slots) -> void;

	auto TST_GPU_API resource_descriptor_heap_free_buffer_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void;
	auto TST_GPU_API resource_descriptor_heap_free_buffer_slots(GPUContext *                  p_ctx, ResourceDescriptorHeapHandle p_resource_heap,
																InitialiserList<const uint32> p_heap_slots) -> void;
	auto TST_GPU_API resource_descriptor_heap_free_texture_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void;
	auto TST_GPU_API resource_descriptor_heap_free_texture_slots(GPUContext *                  p_ctx, ResourceDescriptorHeapHandle p_resource_heap,
																 InitialiserList<const uint32> p_heap_slots) -> void;
	auto TST_GPU_API sampler_descriptor_heap_free_sampler_slot(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot) -> void;
	auto TST_GPU_API sampler_descriptor_heap_free_sampler_slots(GPUContext *                  p_ctx, SamplerDescriptorHeapHandle p_sampler_heap,
																InitialiserList<const uint32> p_heap_slots) -> void;

	auto TST_GPU_API resource_descriptor_heap_write_buffer(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot,
														   BufferHandle p_buffer) -> void;
	auto TST_GPU_API resource_descriptor_heap_write_buffers(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, InitialiserList<const uint32> p_heap_slots,
															InitialiserList<const BufferHandle> p_buffers) -> void;
	// Set p_mip_levels to c_all_mip_levels to access the entire texture's mip levels
	auto TST_GPU_API resource_descriptor_heap_write_texture(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, TextureHandle p_texture,
															bool p_storage, uint32 p_mip = c_allMipLevels) -> void;
	auto TST_GPU_API resource_descriptor_heap_write_textures(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, InitialiserList<const uint32> p_heap_slots,
															 InitialiserList<const TextureHandle> p_textures, InitialiserList<const bool> p_storage,
															 InitialiserList<const uint32> p_mip_levels) -> void;
	auto TST_GPU_API sampler_descriptor_heap_write_sampler(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot,
														   const SamplerDesc &p_sampler_desc) -> void;
	auto TST_GPU_API sampler_descriptor_heap_write_samplers(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, InitialiserList<const uint32> p_heap_slots,
															InitialiserList<const SamplerDesc> p_sampler_descs) -> void;

	#pragma endregion

	#pragma region buffer

	[[nodiscard]] auto TST_GPU_API buffer_create(GPUContext *p_ctx, const BufferDesc &p_desc) -> BufferHandle;
	// If p_out_buffers is > 1, then p_descs can be 1, in which case it will create N identical buffers, otherwise p_descs.size() has to equal p_out_buffers.size()
	auto TST_GPU_API buffer_create_n(GPUContext *p_ctx, InitialiserList<const BufferDesc> p_descs, std::span<BufferHandle> p_out_buffers) -> void;

	auto TST_GPU_API buffer_destroy(GPUContext *p_ctx, BufferHandle p_buffer) -> void;
	auto TST_GPU_API buffer_destroy_n(GPUContext *p_ctx, InitialiserList<const BufferHandle> p_buffers) -> void;

	auto TST_GPU_API buffer_write_data(GPUContext *p_ctx, BufferHandle p_buffer, const void *p_data, uint64 p_size, uint64 p_offset = 0u) -> void; // If host-visible

	[[nodiscard]] auto TST_GPU_API buffer_get_mapped_data(GPUContext *p_ctx, BufferHandle p_buffer) -> void *;
	auto TST_GPU_API buffer_get_mapped_data_n(GPUContext *p_ctx, InitialiserList<const BufferHandle> p_buffers, std::span<void *> p_out_mapped_data) -> void;

	[[nodiscard]] auto TST_GPU_API buffer_get_address(GPUContext *p_ctx, BufferHandle p_buffer) -> DeviceAddress;
	auto TST_GPU_API buffer_get_address_n(GPUContext *p_ctx, InitialiserList<const BufferHandle> p_buffers, std::span<DeviceAddress> p_out_device_addresses) -> void;

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
	[[nodiscard]] auto TST_GPU_API surface_create(GPUContext *p_ctx, void *p_hwnd) -> SurfaceHandle;
	auto TST_GPU_API               surface_destroy(GPUContext *p_ctx, SurfaceHandle p_surface) -> void;

	#pragma region synchronisation

	// Secretly a timeline semaphore...
	// After finally overcoming my fear of timeline semaphores, I awakened to the fact that they are objectively better than fences and binary semaphores altogether.
	// (Again, except for CERTAIN swapchain operations, which mandate the usage of binary semaphores). This realisation came to me while using DirectX 12.
	// If you are familiar, you will know that while swapchain usage is heavily simplified, you will still have to manage concurrent gpu work jobs over multiple frames
	// using ID3D12Fences. Little to my knowledge at the time, these were practically identical to Vulkan's timeline semaphores. So when I came back to Vulkan after my
	// DirectX 12 side quest, I finally understood where all the hype around timeline semaphores came from.
	// Now I am a proud advocate against the use of vk::Fences in favour of timeline semaphores
	[[nodiscard]] auto TST_GPU_API semaphore_create(GPUContext *p_ctx, uint64 p_initial_value = 0u) -> SemaphoreHandle;
	auto TST_GPU_API semaphore_create_n(GPUContext *p_ctx, InitialiserList<const uint64> p_initial_values, std::span<SemaphoreHandle> p_out_semaphores) -> void;

	auto TST_GPU_API semaphore_destroy(GPUContext *p_ctx, SemaphoreHandle p_semaphore) -> void;
	auto TST_GPU_API semaphore_destroy_n(GPUContext *p_ctx, InitialiserList<const SemaphoreHandle> p_semaphores) -> void;

	// Waits for N semaphores
	auto TST_GPU_API semaphore_wait_n(GPUContext *p_ctx, InitialiserList<const SemaphoreHandle> p_semaphores, InitialiserList<const uint64> p_values) -> void;

	// Returns the current counter value of the semaphore
	auto TST_GPU_API semaphore_get_value(GPUContext *p_ctx, SemaphoreHandle p_semaphore) -> uint64;
	// Retrieves the values from N semaphores
	auto TST_GPU_API semaphore_get_value_n(GPUContext *p_ctx, InitialiserList<const SemaphoreHandle> p_semaphores, std::span<uint64> p_out_values) -> void;

	#pragma endregion

	#pragma region texture

	// I don't think you will ever need to use this...
	auto TST_GPU_API _get_num_undefined_textures(GPUContext *p_ctx) -> uint64;

	[[nodiscard]] auto TST_GPU_API texture_create(GPUContext *p_ctx, const TextureDesc &p_desc) -> TextureHandle;
	auto TST_GPU_API               texture_create_n(GPUContext *p_ctx, InitialiserList<const TextureDesc> p_descs, std::span<TextureHandle> p_out_textures) -> void;

	auto TST_GPU_API texture_destroy(GPUContext *p_ctx, TextureHandle p_texture) -> void;
	auto TST_GPU_API texture_destroy_n(GPUContext *p_ctx, InitialiserList<const TextureHandle> p_textures) -> void;

	// Register a texture (just returns a unique handle) no gpu logic will be executed.
	// This is useful for multithreaded loading where you want to target a specific texture, but you have not yet created it...
	[[nodiscard]] auto TST_GPU_API texture_register_null(GPUContext *p_ctx) -> TextureHandle;
	auto TST_GPU_API               texture_register_null_n(GPUContext *p_ctx, std::span<TextureHandle> p_out_textures) -> void;
	// This can only validly be called at some point after texture_register_null. This process is essentially just a deferred creation and is useful.
	// Secretly, texture_create(...) is actually just texture_register_null(...) followed by texture_create_into(...)!
	auto TST_GPU_API texture_create_into(GPUContext *p_ctx, TextureHandle p_dst_texture, const TextureDesc &p_desc) -> void;
	auto TST_GPU_API texture_create_into_n(GPUContext *p_ctx, InitialiserList<const TextureDesc> p_descs, std::span<TextureHandle> p_dst_textures) -> void;

	[[nodiscard]] auto TST_GPU_API texture_get_desc(GPUContext *p_ctx, TextureHandle p_texture) -> const TextureDesc &;

	#pragma endregion

	#pragma region swapchain

	[[nodiscard]] auto TST_GPU_API swapchain_create(GPUContext *p_ctx, SurfaceHandle p_surface, math::UInt2 p_initial_extent) -> SwapchainHandle;
	auto TST_GPU_API               swapchain_destroy(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> void;

	auto TST_GPU_API swapchain_set_max_framerate(GPUContext *p_ctx, SwapchainHandle p_swapchain, uint32 p_framerate = UINT32_MAX) -> void; // UINT32_MAX = uncapped
	auto TST_GPU_API swapchain_wait_for_framerate(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> void;
	auto TST_GPU_API swapchain_set_frame_marker(GPUContext *p_ctx, SwapchainHandle p_swapchain, EFrameMarker p_marker) -> void;

	// Returns true if successful, false otherwise. Use this to skip rendering if unsuccessful and retry
	auto TST_GPU_API swapchain_resize(GPUContext *p_ctx, SwapchainHandle p_swapchain, math::UInt2 p_extent) -> bool; // Resize is identical to 'Recreate'...

	[[nodiscard]] auto TST_GPU_API swapchain_acquire_next_image(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> TextureHandle;
	// If the image was unable to be acquired. Returns a null handle

	// Fancy way of saying present src to general
	auto TST_GPU_API swapchain_insert_pre_render_resource_barrier(GPUContext *p_ctx, CommandListHandle p_cmd_list, TextureHandle p_attachment_texture) -> void;

	// Returns true if successful, false if the present operation was unsuccessful. Recreate if false
	auto TST_GPU_API swapchain_submit_and_present(GPUContext *               p_ctx, SwapchainHandle p_swapchain, CommandListHandle p_cmd_list,
												  const SemaphoreSubmitInfo &p_signal_semaphore_info) -> bool;
	auto TST_GPU_API swapchain_submit_and_present(GPUContext *                               p_ctx, SwapchainHandle p_swapchain, CommandListHandle p_cmd_list,
												  const SemaphoreSubmitInfo &                p_signal_semaphore_info,
												  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos) -> bool;

	#pragma endregion

	#pragma region shader

	[[nodiscard]] auto TST_GPU_API shader_create(GPUContext *p_ctx, const ShaderDesc &p_desc) -> ShaderHandle;
	// If p_link is true, the shaders will be linked together, see https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateShadersEXT.html for more info.
	auto TST_GPU_API shader_create_n(GPUContext *p_ctx, std::span<ShaderHandle> p_out_shaders, InitialiserList<const ShaderDesc> p_shader_descs,
									 bool        p_link = false) -> void;

	auto TST_GPU_API shader_destroy(GPUContext *p_ctx, ShaderHandle p_shader) -> void;
	auto TST_GPU_API shader_destroy_n(GPUContext *p_ctx, InitialiserList<const ShaderHandle> p_shaders) -> void;

	#pragma endregion
}
