#include "toast_gpu/api.hpp"

#include <algorithm>
#include <print>
#include <ranges>
#include <threads.h>

#include "toast_lib/format.hpp"
#include "toast_lib/freelist_allocator.hpp"
#include "toast_lib/pool.hpp"
#include "toast_lib/set.hpp"

#include <volk.h>
#include <vk_mem_alloc.h>

#include "toast_lib/map.hpp"

#undef min
#undef max

namespace toaster::gpu
{
	struct QueueFamilyIndices
	{
		[[nodiscard]] constexpr auto get(EQueueType p_queue_type) const -> uint32
		{
			switch (p_queue_type)
			{
				case EQueueType::eGraphics: return graphics;
				case EQueueType::eCompute: return compute;
				case EQueueType::eTransfer: return transfer;
				case EQueueType::eMax: return UINT32_MAX;
			}
			return UINT32_MAX;
		}

		uint32 graphics{UINT32_MAX};
		uint32 compute{UINT32_MAX};
		uint32 transfer{UINT32_MAX};
	};

	struct ResourceDescriptorHeap
	{
		mtx_t bufferSlotMutex{};
		mtx_t imageSlotMutex{};

		VkBindHeapInfoEXT bindInfo{};

		VkBuffer      heapBuffer{nullptr};
		VmaAllocation heapAllocation{nullptr};
		void *        mappedHeapMemory{nullptr};

		FreelistAllocator<uint32> bufferSlotAllocator;
		FreelistAllocator<uint32> imageSlotAllocator;

		VkDeviceSize bufferDescriptorSize{0u};
		VkDeviceSize imageDescriptorSize{0u};

		VkDeviceSize bufferSegmentSize{0u};

		uint64       imageSegmentOffset{0u};
		VkDeviceSize imageSegmentSize{0u};
	};

	struct SamplerDescriptorHeap
	{
		mtx_t samplerSlotMutex{};

		VkBindHeapInfoEXT bindInfo{};

		VkBuffer      heapBuffer{nullptr};
		VmaAllocation heapAllocation{nullptr};
		void *        mappedHeapMemory{nullptr};

		FreelistAllocator<uint32> samplerSlotAllocator;

		VkDeviceSize samplerDescriptorSize{0u};
	};

	struct CommandList
	{
		VkCommandBuffer cmd{nullptr};
		VkCommandPool   pool{nullptr};

		EQueueType queueType{EQueueType::eMax};

		bool open{false};
		bool secondary{false};
	};

	struct Semaphore
	{
		VkSemaphore semaphore{nullptr};
	};

	struct Buffer
	{
		VkBuffer      buffer{nullptr};
		VmaAllocation allocation{nullptr};
		uint64        size{0u};

		DeviceAddress     address{0u};
		void *            mapped{nullptr};
		EBufferUsageFlags usageFlags{0u};
	};

	struct Texture
	{
		VkImage       image{nullptr};
		VkImageView   imageView{nullptr}; // Not necessary unless the texture is a render target
		VmaAllocation allocation{nullptr};

		TextureDesc desc; // Useful

		bool isSwapchainImage{false}; // Used to know whether the texture owns it's image and should create/destroy it
	};

	struct Surface
	{
		VkSurfaceKHR surface{nullptr};
		HWND         hwnd{nullptr};
	};

	struct Swapchain
	{
		Vector<TextureHandle> attachments;
		Vector<VkSemaphore>   imageAvailableSemaphores; // The max number of concurrent gpu swapchain workloads
		Vector<VkSemaphore>   renderFinishedSemaphores; // PSI

		VkSwapchainKHR swapchain{nullptr};
		SurfaceHandle  surface{nullptr};

		SemaphoreHandle latencySemaphore{nullptr};

		uint64      frameId{0ull};
		math::UInt2 extent{0u, 0u};

		uint32 acquisitonIndex{0u}; // This can be different from the frame index. But 99% of the time it will be the same
		uint32 imageIndex{0u};

		VkSurfaceFormatKHR surfaceFormat{};
		uint32             minImageCount{0u};
		VkPresentModeKHR   presentMode{VK_PRESENT_MODE_FIFO_KHR};
	};

	struct Shader
	{
		VkShaderEXT shader{nullptr};

		VkShaderStageFlagBits stage{static_cast<VkShaderStageFlagBits>(0u)};
		VkShaderStageFlags    nextStage{0u};
	};

	#define TST_REGISTER_RESOURCE_POOL(__resourceType, __name) struct  Pool<__resourceType>  __name

	struct GPUContext
	{
		bool enableDebugInfo{true};
		bool usingSwapchain{false}; // From the desc

		uint32 maxConcurrentSwapchainWorkloads{3u};

		VkInstance vulkanInstance{nullptr};

		VkPhysicalDevice                            physicalDevice{nullptr};
		VkPhysicalDeviceProperties2                 deviceProperties;
		VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties;

		VkDevice logicalDevice{nullptr};

		QueueFamilyIndices                                 queueFamilyIndices{};
		Array<VkQueue, TST_LIB_ENUM_VAL(EQueueType::eMax)> queues;
		mtx_t                                              queueMutexes[TST_LIB_ENUM_VAL(EQueueType::eMax)];

		VmaAllocator allocator{nullptr};

		TST_REGISTER_RESOURCE_POOL(CommandList, commandLists);
		mtx_t                                                                freeCommandListMutex;
		Array<Vector<CommandListHandle>, TST_LIB_ENUM_VAL(EQueueType::eMax)> freeCommandLists; // Free command lists per queue type
		Array<Vector<CommandListHandle>, TST_LIB_ENUM_VAL(EQueueType::eMax)> freeSecondaryLists; // Free command lists per queue type
		Array<VkCommandPool, TST_LIB_ENUM_VAL(EQueueType::eMax)>             transientPools; // Mostly used for allocating command buffers for image layout transitions

		// The page size should be 1 because you will only ever have one of each type of heap
		Pool<ResourceDescriptorHeap, 1u> resourceHeaps;
		Pool<SamplerDescriptorHeap, 1u>  samplerHeaps;

		TST_REGISTER_RESOURCE_POOL(Texture, textures);
		UnorderedSet<TextureHandle> undefinedTextures; // All the textures that are in the undefined format and are awaiting a layout transition
		mtx_t                       undefinedTexturesMutex;

		TST_REGISTER_RESOURCE_POOL(Semaphore, semaphores);
		TST_REGISTER_RESOURCE_POOL(Buffer, buffers);
		TST_REGISTER_RESOURCE_POOL(Surface, surfaces);
		TST_REGISTER_RESOURCE_POOL(Swapchain, swapchains);
		TST_REGISTER_RESOURCE_POOL(Shader, shaders);
	};

	// static APIImpl *p_ctx{nullptr};
	auto to_string(VkDebugUtilsMessageTypeFlagsEXT p_message_type) -> String
	{
		switch (p_message_type)
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "Device Address Binding";
			default: break;
		}
		return "Unknown";
	}

	static auto vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      p_message_severity, VkDebugUtilsMessageTypeFlagsEXT p_message_type,
								  const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, [[maybe_unused]] void *            p_user_data) -> VkBool32
	{
		switch (p_message_severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				std::printf("[Verbose] | Validation layer: %s | Message: %s", to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				std::printf("[Info] | Validation layer: %s | Message: %s", to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				std::printf("[Warning] | Validation layer: %s | Message: %s", to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				std::printf("[Error] | Validation layer: %s | Message: %s", to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			default: break;
		}
		return false;
	}

	#define TST_GPU_VK_CHECK_RESULT(__expr) do { if(__expr != VK_SUCCESS) { TST_PERMA_ASSERT(false); } } while(false)
	#define TST_GPU_VK_CHECK_RESULT_MSG(__expr, __msg) do { if(__expr != VK_SUCCESS) { TST_PERMA_ASSERT_MSG(false, __msg); } } while(false)

	#ifndef NDEBUG
	#define TST_GPU_VK_CHECK_RESULT_DEBUG(__expr) TST_GPU_VK_CHECK_RESULT(__expr)
	#define TST_GPU_VK_CHECK_RESULT_MSG_DEBUG(__expr, __msg) TST_GPU_VK_CHECK_RESULT_MSG(__expr, __msg)
	#else
	#define TST_GPU_VK_CHECK_RESULT_DEBUG(__expr) (void)__expr
	#define TST_GPU_VK_CHECK_RESULT_MSG_DEBUG(__expr, __msg) (void)__expr
	#endif

	#define STYPE(__type) VK_STRUCTURE_TYPE_##__type

	auto is_device_suitable(VkPhysicalDevice p_physical_device, std::span<CString> p_required_extensions) -> bool
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(p_physical_device, &device_properties);

		// We need Vulkan 1.4 for the descriptor heap extension.
		if (device_properties.apiVersion < VK_API_VERSION_1_4)
			return false;

		uint32 queue_family_property_count;
		vkGetPhysicalDeviceQueueFamilyProperties(p_physical_device, &queue_family_property_count, nullptr);
		Vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);
		vkGetPhysicalDeviceQueueFamilyProperties(p_physical_device, &queue_family_property_count, queue_family_properties.data());

		const bool supports_graphics{
			std::ranges::any_of(queue_family_properties, [](const auto &queue_family) -> bool
			{
				return !!(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
			})
		};

		const bool supports_compute{
			std::ranges::any_of(queue_family_properties, [](const auto &queue_family) -> bool
			{
				return !!(queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT);
			})
		};

		if (!supports_graphics || !supports_compute)
			return false;

		uint32 extension_property_count;
		vkEnumerateDeviceExtensionProperties(p_physical_device, nullptr, &extension_property_count, nullptr);
		Vector<VkExtensionProperties> extension_properties(extension_property_count);
		vkEnumerateDeviceExtensionProperties(p_physical_device, nullptr, &extension_property_count, extension_properties.data());

		const bool supports_all_required_device_extensions{
			std::ranges::all_of(p_required_extensions, [&extension_properties](CString p_required_ext)
			{
				return std::ranges::any_of(extension_properties, [p_required_ext](const VkExtensionProperties &p_available_ext)
				{
					return std::strcmp(p_available_ext.extensionName, p_required_ext) == 0;
				});
			})
		};

		return supports_all_required_device_extensions;
	}

	auto select_queue_family_indices(VkPhysicalDevice p_physical_device, bool p_use_present) -> QueueFamilyIndices
	{
		uint32 queue_family_property_count;
		vkGetPhysicalDeviceQueueFamilyProperties(p_physical_device, &queue_family_property_count, nullptr);
		Vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);
		vkGetPhysicalDeviceQueueFamilyProperties(p_physical_device, &queue_family_property_count, queue_family_properties.data());

		QueueFamilyIndices queue_family_indices{};

		for (uint32 i{0u}; i < queue_family_property_count; ++i)
		{
			const auto queue_flags{queue_family_properties[i].queueFlags};

			// Get a queue that supports graphics, compute and presentation
			if (queue_flags & VK_QUEUE_GRAPHICS_BIT && queue_flags & VK_QUEUE_COMPUTE_BIT && (p_use_present
																								  ? vkGetPhysicalDeviceWin32PresentationSupportKHR(p_physical_device, i)
																								  : true))
			{
				queue_family_indices.graphics = i;
				queue_family_indices.compute  = i;
			}

			// Dedicated transfer queue
			if (queue_flags & VK_QUEUE_TRANSFER_BIT && !(queue_flags & VK_QUEUE_GRAPHICS_BIT) && !(queue_flags & VK_QUEUE_COMPUTE_BIT))
				queue_family_indices.transfer = i;
		}

		return queue_family_indices;
	}

	#define TST_GPU_FLAG_COMBINE(__tst_type, __vk_type) out_flags |= (p_flags & __tst_type) ? __vk_type : 0u

	constexpr auto get_vk_format(EFormat p_format) -> VkFormat
	{
		switch (p_format)
		{
			case EFormat::eUndefined: return VK_FORMAT_UNDEFINED;
			case EFormat::eR8Unorm: return VK_FORMAT_R8_UNORM;
			case EFormat::eR8Snorm: return VK_FORMAT_R8_SNORM;
			case EFormat::eR8Uint: return VK_FORMAT_R8_UINT;
			case EFormat::eR8Sint: return VK_FORMAT_R8_SINT;
			case EFormat::eR8Srgb: return VK_FORMAT_R8_SRGB;
			case EFormat::eR8G8Unorm: return VK_FORMAT_R8G8_UNORM;
			case EFormat::eR8G8Snorm: return VK_FORMAT_R8G8_SNORM;
			case EFormat::eR8G8Uint: return VK_FORMAT_R8G8_UINT;
			case EFormat::eR8G8Sint: return VK_FORMAT_R8G8_SINT;
			case EFormat::eR8G8Srgb: return VK_FORMAT_R8G8_SRGB;
			case EFormat::eR8G8B8Unorm: return VK_FORMAT_R8G8B8_UNORM;
			case EFormat::eR8G8B8Snorm: return VK_FORMAT_R8G8B8_SNORM;
			case EFormat::eR8G8B8Uint: return VK_FORMAT_R8G8B8_UINT;
			case EFormat::eR8G8B8Sint: return VK_FORMAT_R8G8B8_SINT;
			case EFormat::eR8G8B8Srgb: return VK_FORMAT_R8G8B8_SRGB;
			case EFormat::eB8G8R8Unorm: return VK_FORMAT_B8G8R8_UNORM;
			case EFormat::eB8G8R8Snorm: return VK_FORMAT_B8G8R8_SNORM;
			case EFormat::eB8G8R8Uint: return VK_FORMAT_B8G8R8_UINT;
			case EFormat::eB8G8R8Sint: return VK_FORMAT_B8G8R8_SINT;
			case EFormat::eB8G8R8Srgb: return VK_FORMAT_B8G8R8_SRGB;
			case EFormat::eR8G8B8A8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
			case EFormat::eR8G8B8A8Snorm: return VK_FORMAT_R8G8B8A8_SNORM;
			case EFormat::eR8G8B8A8Uint: return VK_FORMAT_R8G8B8A8_UINT;
			case EFormat::eR8G8B8A8Sint: return VK_FORMAT_R8G8B8A8_SINT;
			case EFormat::eR8G8B8A8Srgb: return VK_FORMAT_R8G8B8A8_SRGB;
			case EFormat::eB8G8R8A8Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
			case EFormat::eB8G8R8A8Snorm: return VK_FORMAT_B8G8R8A8_SNORM;
			case EFormat::eB8G8R8A8Uint: return VK_FORMAT_B8G8R8A8_UINT;
			case EFormat::eB8G8R8A8Sint: return VK_FORMAT_B8G8R8A8_SINT;
			case EFormat::eB8G8R8A8Srgb: return VK_FORMAT_B8G8R8A8_SRGB;
			case EFormat::eR16Unorm: return VK_FORMAT_R16_UNORM;
			case EFormat::eR16Snorm: return VK_FORMAT_R16_SNORM;
			case EFormat::eR16Uint: return VK_FORMAT_R16_UINT;
			case EFormat::eR16Sint: return VK_FORMAT_R16_SINT;
			case EFormat::eR16Sfloat: return VK_FORMAT_R16_SFLOAT;
			case EFormat::eR16G16Unorm: return VK_FORMAT_R16G16_UNORM;
			case EFormat::eR16G16Snorm: return VK_FORMAT_R16G16_SNORM;
			case EFormat::eR16G16Uint: return VK_FORMAT_R16G16_UINT;
			case EFormat::eR16G16Sint: return VK_FORMAT_R16G16_SINT;
			case EFormat::eR16G16Sfloat: return VK_FORMAT_R16G16_SFLOAT;
			case EFormat::eR16G16B16Unorm: return VK_FORMAT_R16G16B16_UNORM;
			case EFormat::eR16G16B16Snorm: return VK_FORMAT_R16G16B16_SNORM;
			case EFormat::eR16G16B16Uint: return VK_FORMAT_R16G16B16_UINT;
			case EFormat::eR16G16B16Sint: return VK_FORMAT_R16G16B16_SINT;
			case EFormat::eR16G16B16Sfloat: return VK_FORMAT_R16G16B16_SFLOAT;
			case EFormat::eR16G16B16A16Unorm: return VK_FORMAT_R16G16B16A16_UNORM;
			case EFormat::eR16G16B16A16Snorm: return VK_FORMAT_R16G16B16A16_SNORM;
			case EFormat::eR16G16B16A16Uint: return VK_FORMAT_R16G16B16A16_UINT;
			case EFormat::eR16G16B16A16Sint: return VK_FORMAT_R16G16B16A16_SINT;
			case EFormat::eR16G16B16A16Sfloat: return VK_FORMAT_R16G16B16A16_SFLOAT;
			case EFormat::eR32Uint: return VK_FORMAT_R32_UINT;
			case EFormat::eR32Sint: return VK_FORMAT_R32_SINT;
			case EFormat::eR32Sfloat: return VK_FORMAT_R32_SFLOAT;
			case EFormat::eR32G32Uint: return VK_FORMAT_R32G32_UINT;
			case EFormat::eR32G32Sint: return VK_FORMAT_R32G32_SINT;
			case EFormat::eR32G32Sfloat: return VK_FORMAT_R32G32_SFLOAT;
			case EFormat::eR32G32B32Uint: return VK_FORMAT_R32G32B32_UINT;
			case EFormat::eR32G32B32Sint: return VK_FORMAT_R32G32B32_SINT;
			case EFormat::eR32G32B32Sfloat: return VK_FORMAT_R32G32B32_SFLOAT;
			case EFormat::eR32G32B32A32Uint: return VK_FORMAT_R32G32B32A32_UINT;
			case EFormat::eR32G32B32A32Sint: return VK_FORMAT_R32G32B32A32_SINT;
			case EFormat::eR32G32B32A32Sfloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case EFormat::eR64Uint: return VK_FORMAT_R64_UINT;
			case EFormat::eR64Sint: return VK_FORMAT_R64_SINT;
			case EFormat::eR64Sfloat: return VK_FORMAT_R64_SFLOAT;
			case EFormat::eR64G64Uint: return VK_FORMAT_R64G64_UINT;
			case EFormat::eR64G64Sint: return VK_FORMAT_R64G64_SINT;
			case EFormat::eR64G64Sfloat: return VK_FORMAT_R64G64_SFLOAT;
			case EFormat::eR64G64B64Uint: return VK_FORMAT_R64G64B64_UINT;
			case EFormat::eR64G64B64Sint: return VK_FORMAT_R64G64B64_SINT;
			case EFormat::eR64G64B64Sfloat: return VK_FORMAT_R64G64B64_SFLOAT;
			case EFormat::eR64G64B64A64Uint: return VK_FORMAT_R64G64B64A64_UINT;
			case EFormat::eR64G64B64A64Sint: return VK_FORMAT_R64G64B64A64_SINT;
			case EFormat::eR64G64B64A64Sfloat: return VK_FORMAT_R64G64B64A64_SFLOAT;
			case EFormat::eD16Unorm: return VK_FORMAT_D16_UNORM;
			case EFormat::eD32Sfloat: return VK_FORMAT_D32_SFLOAT;
			case EFormat::eS8Uint: return VK_FORMAT_S8_UINT;
			case EFormat::eD16UnormS8Uint: return VK_FORMAT_D16_UNORM_S8_UINT;
			case EFormat::eD24UnormS8Uint: return VK_FORMAT_D24_UNORM_S8_UINT;
			case EFormat::eD32SfloatS8Uint: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		}
		return VK_FORMAT_UNDEFINED;
	}

	constexpr auto get_tst_format(VkFormat p_format) -> EFormat
	{
		switch (p_format)
		{
			case VK_FORMAT_UNDEFINED: return EFormat::eUndefined;
			case VK_FORMAT_R8_UNORM: return EFormat::eR8Unorm;
			case VK_FORMAT_R8_SNORM: return EFormat::eR8Snorm;
			case VK_FORMAT_R8_UINT: return EFormat::eR8Uint;
			case VK_FORMAT_R8_SINT: return EFormat::eR8Sint;
			case VK_FORMAT_R8_SRGB: return EFormat::eR8Srgb;
			case VK_FORMAT_R8G8_UNORM: return EFormat::eR8G8Unorm;
			case VK_FORMAT_R8G8_SNORM: return EFormat::eR8G8Snorm;
			case VK_FORMAT_R8G8_UINT: return EFormat::eR8G8Uint;
			case VK_FORMAT_R8G8_SINT: return EFormat::eR8G8Sint;
			case VK_FORMAT_R8G8_SRGB: return EFormat::eR8G8Srgb;
			case VK_FORMAT_R8G8B8_UNORM: return EFormat::eR8G8B8Unorm;
			case VK_FORMAT_R8G8B8_SNORM: return EFormat::eR8G8B8Snorm;
			case VK_FORMAT_R8G8B8_UINT: return EFormat::eR8G8B8Uint;
			case VK_FORMAT_R8G8B8_SINT: return EFormat::eR8G8B8Sint;
			case VK_FORMAT_R8G8B8_SRGB: return EFormat::eR8G8B8Srgb;
			case VK_FORMAT_B8G8R8_UNORM: return EFormat::eB8G8R8Unorm;
			case VK_FORMAT_B8G8R8_SNORM: return EFormat::eB8G8R8Snorm;
			case VK_FORMAT_B8G8R8_UINT: return EFormat::eB8G8R8Uint;
			case VK_FORMAT_B8G8R8_SINT: return EFormat::eB8G8R8Sint;
			case VK_FORMAT_B8G8R8_SRGB: return EFormat::eB8G8R8Srgb;
			case VK_FORMAT_R8G8B8A8_UNORM: return EFormat::eR8G8B8A8Unorm;
			case VK_FORMAT_R8G8B8A8_SNORM: return EFormat::eR8G8B8A8Snorm;
			case VK_FORMAT_R8G8B8A8_UINT: return EFormat::eR8G8B8A8Uint;
			case VK_FORMAT_R8G8B8A8_SINT: return EFormat::eR8G8B8A8Sint;
			case VK_FORMAT_R8G8B8A8_SRGB: return EFormat::eR8G8B8A8Srgb;
			case VK_FORMAT_B8G8R8A8_UNORM: return EFormat::eB8G8R8A8Unorm;
			case VK_FORMAT_B8G8R8A8_SNORM: return EFormat::eB8G8R8A8Snorm;
			case VK_FORMAT_B8G8R8A8_UINT: return EFormat::eB8G8R8A8Uint;
			case VK_FORMAT_B8G8R8A8_SINT: return EFormat::eB8G8R8A8Sint;
			case VK_FORMAT_B8G8R8A8_SRGB: return EFormat::eB8G8R8A8Srgb;
			case VK_FORMAT_R16_UNORM: return EFormat::eR16Unorm;
			case VK_FORMAT_R16_SNORM: return EFormat::eR16Snorm;
			case VK_FORMAT_R16_UINT: return EFormat::eR16Uint;
			case VK_FORMAT_R16_SINT: return EFormat::eR16Sint;
			case VK_FORMAT_R16_SFLOAT: return EFormat::eR16Sfloat;
			case VK_FORMAT_R16G16_UNORM: return EFormat::eR16G16Unorm;
			case VK_FORMAT_R16G16_SNORM: return EFormat::eR16G16Snorm;
			case VK_FORMAT_R16G16_UINT: return EFormat::eR16G16Uint;
			case VK_FORMAT_R16G16_SINT: return EFormat::eR16G16Sint;
			case VK_FORMAT_R16G16_SFLOAT: return EFormat::eR16G16Sfloat;
			case VK_FORMAT_R16G16B16_UNORM: return EFormat::eR16G16B16Unorm;
			case VK_FORMAT_R16G16B16_SNORM: return EFormat::eR16G16B16Snorm;
			case VK_FORMAT_R16G16B16_UINT: return EFormat::eR16G16B16Uint;
			case VK_FORMAT_R16G16B16_SINT: return EFormat::eR16G16B16Sint;
			case VK_FORMAT_R16G16B16_SFLOAT: return EFormat::eR16G16B16Sfloat;
			case VK_FORMAT_R16G16B16A16_UNORM: return EFormat::eR16G16B16A16Unorm;
			case VK_FORMAT_R16G16B16A16_SNORM: return EFormat::eR16G16B16A16Snorm;
			case VK_FORMAT_R16G16B16A16_UINT: return EFormat::eR16G16B16A16Uint;
			case VK_FORMAT_R16G16B16A16_SINT: return EFormat::eR16G16B16A16Sint;
			case VK_FORMAT_R16G16B16A16_SFLOAT: return EFormat::eR16G16B16A16Sfloat;
			case VK_FORMAT_R32_UINT: return EFormat::eR32Uint;
			case VK_FORMAT_R32_SINT: return EFormat::eR32Sint;
			case VK_FORMAT_R32_SFLOAT: return EFormat::eR32Sfloat;
			case VK_FORMAT_R32G32_UINT: return EFormat::eR32G32Uint;
			case VK_FORMAT_R32G32_SINT: return EFormat::eR32G32Sint;
			case VK_FORMAT_R32G32_SFLOAT: return EFormat::eR32G32Sfloat;
			case VK_FORMAT_R32G32B32_UINT: return EFormat::eR32G32B32Uint;
			case VK_FORMAT_R32G32B32_SINT: return EFormat::eR32G32B32Sint;
			case VK_FORMAT_R32G32B32_SFLOAT: return EFormat::eR32G32B32Sfloat;
			case VK_FORMAT_R32G32B32A32_UINT: return EFormat::eR32G32B32A32Uint;
			case VK_FORMAT_R32G32B32A32_SINT: return EFormat::eR32G32B32A32Sint;
			case VK_FORMAT_R32G32B32A32_SFLOAT: return EFormat::eR32G32B32A32Sfloat;
			case VK_FORMAT_R64_UINT: return EFormat::eR64Uint;
			case VK_FORMAT_R64_SINT: return EFormat::eR64Sint;
			case VK_FORMAT_R64_SFLOAT: return EFormat::eR64Sfloat;
			case VK_FORMAT_R64G64_UINT: return EFormat::eR64G64Uint;
			case VK_FORMAT_R64G64_SINT: return EFormat::eR64G64Sint;
			case VK_FORMAT_R64G64_SFLOAT: return EFormat::eR64G64Sfloat;
			case VK_FORMAT_R64G64B64_UINT: return EFormat::eR64G64B64Uint;
			case VK_FORMAT_R64G64B64_SINT: return EFormat::eR64G64B64Sint;
			case VK_FORMAT_R64G64B64_SFLOAT: return EFormat::eR64G64B64Sfloat;
			case VK_FORMAT_R64G64B64A64_UINT: return EFormat::eR64G64B64A64Uint;
			case VK_FORMAT_R64G64B64A64_SINT: return EFormat::eR64G64B64A64Sint;
			case VK_FORMAT_R64G64B64A64_SFLOAT: return EFormat::eR64G64B64A64Sfloat;
			case VK_FORMAT_D16_UNORM: return EFormat::eD16Unorm;
			case VK_FORMAT_D32_SFLOAT: return EFormat::eD32Sfloat;
			case VK_FORMAT_S8_UINT: return EFormat::eS8Uint;
			case VK_FORMAT_D16_UNORM_S8_UINT: return EFormat::eD16UnormS8Uint;
			case VK_FORMAT_D24_UNORM_S8_UINT: return EFormat::eD24UnormS8Uint;
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return EFormat::eD32SfloatS8Uint;
			default: return EFormat::eUndefined;
		}
		return EFormat::eUndefined;
	}

	constexpr auto has_stencil_component(EFormat p_format) -> bool
	{
		return p_format == EFormat::eD32SfloatS8Uint || p_format == EFormat::eD24UnormS8Uint;
	}

	constexpr auto is_depth_format(EFormat p_format) -> bool
	{
		return p_format == EFormat::eD16Unorm || p_format == EFormat::eD16UnormS8Uint || p_format == EFormat::eD24UnormS8Uint || p_format == EFormat::eD32Sfloat ||
			   p_format == EFormat::eD32SfloatS8Uint;
	}

	constexpr auto get_vk_image_aspect_mask(EFormat p_format) -> VkImageAspectFlags
	{
		VkImageAspectFlags aspect_mask = is_depth_format(p_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		aspect_mask                    |= has_stencil_component(p_format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u;
		return aspect_mask;
	}

	auto get_vk_load_op(EAttachmentUsageOP p_usage_op) -> VkAttachmentLoadOp
	{
		switch (p_usage_op)
		{
			case EAttachmentUsageOP::eClearStore:
			case EAttachmentUsageOP::eClearNone:
			case EAttachmentUsageOP::eClearDontCare:
				return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case EAttachmentUsageOP::eLoadStore:
			case EAttachmentUsageOP::eLoadNone:
			case EAttachmentUsageOP::eLoadDontCare:
				return VK_ATTACHMENT_LOAD_OP_LOAD;
			case EAttachmentUsageOP::eNoneStore:
			case EAttachmentUsageOP::eNoneNone:
			case EAttachmentUsageOP::eNoneDontCare:
				return VK_ATTACHMENT_LOAD_OP_NONE;
			case EAttachmentUsageOP::eDontCareStore:
			case EAttachmentUsageOP::eDontCareNone:
			case EAttachmentUsageOP::eDontCareDontCare:
				return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
		return VK_ATTACHMENT_LOAD_OP_NONE;
	}

	auto get_vk_store_op(EAttachmentUsageOP p_usage_op) -> VkAttachmentStoreOp
	{
		switch (p_usage_op)
		{
			case EAttachmentUsageOP::eClearStore:
			case EAttachmentUsageOP::eLoadStore:
			case EAttachmentUsageOP::eNoneStore:
			case EAttachmentUsageOP::eDontCareStore:
				return VK_ATTACHMENT_STORE_OP_STORE;
			case EAttachmentUsageOP::eClearNone:
			case EAttachmentUsageOP::eLoadNone:
			case EAttachmentUsageOP::eNoneNone:
			case EAttachmentUsageOP::eDontCareNone:
				return VK_ATTACHMENT_STORE_OP_NONE;
			case EAttachmentUsageOP::eClearDontCare:
			case EAttachmentUsageOP::eLoadDontCare:
			case EAttachmentUsageOP::eNoneDontCare:
			case EAttachmentUsageOP::eDontCareDontCare:
				return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		return VK_ATTACHMENT_STORE_OP_NONE;
	}

	auto get_vk_resolve_mode(EAttachmentResolveMode p_resolve_mode) -> VkResolveModeFlagBits
	{
		switch (p_resolve_mode)
		{
			case EAttachmentResolveMode::eNone: return VK_RESOLVE_MODE_NONE;
			case EAttachmentResolveMode::eZero: return VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
			case EAttachmentResolveMode::eAverage: return VK_RESOLVE_MODE_AVERAGE_BIT;
			case EAttachmentResolveMode::eMin: return VK_RESOLVE_MODE_MIN_BIT;
			case EAttachmentResolveMode::eMax: return VK_RESOLVE_MODE_MAX_BIT;
		}
		return VK_RESOLVE_MODE_NONE;
	}

	constexpr auto get_vk_shader_stage(EShaderStageFlagBits p_flags) -> VkShaderStageFlagBits
	{
		switch (p_flags)
		{
			case EShaderStageFlagBits::eVertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case EShaderStageFlagBits::ePixel: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case EShaderStageFlagBits::eCompute: return VK_SHADER_STAGE_COMPUTE_BIT;
			case EShaderStageFlagBits::eGeometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
			case EShaderStageFlagBits::eTessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case EShaderStageFlagBits::eTessEval: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case EShaderStageFlagBits::eTask: return VK_SHADER_STAGE_TASK_BIT_EXT;
			case EShaderStageFlagBits::eMesh: return VK_SHADER_STAGE_MESH_BIT_EXT;
			case EShaderStageFlagBits::eNone:
				VK_SHADER_STAGE_VERTEX_BIT;
		}
		return VK_SHADER_STAGE_VERTEX_BIT;
	}

	constexpr auto get_vk_shader_stages(EShaderStageFlags p_flags) -> VkShaderStageFlags
	{
		VkShaderStageFlags out_flags{0ull};

		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eVertex, VK_SHADER_STAGE_VERTEX_BIT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::ePixel, VK_SHADER_STAGE_FRAGMENT_BIT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eCompute, VK_SHADER_STAGE_COMPUTE_BIT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eGeometry, VK_SHADER_STAGE_GEOMETRY_BIT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eTessControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eTessEval, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eTask, VK_SHADER_STAGE_TASK_BIT_EXT);
		TST_GPU_FLAG_COMBINE(EShaderStageFlagBits::eMesh, VK_SHADER_STAGE_MESH_BIT_EXT);

		return out_flags;
	}

	constexpr auto get_vk_sample_count(ESampleCount p_sample_count) -> VkSampleCountFlagBits
	{
		switch (p_sample_count)
		{
			case ESampleCount::e1: return VK_SAMPLE_COUNT_1_BIT;
			case ESampleCount::e2: return VK_SAMPLE_COUNT_2_BIT;
			case ESampleCount::e4: return VK_SAMPLE_COUNT_4_BIT;
			case ESampleCount::e8: return VK_SAMPLE_COUNT_8_BIT;
			case ESampleCount::e16: return VK_SAMPLE_COUNT_16_BIT;
			case ESampleCount::e32: return VK_SAMPLE_COUNT_32_BIT;
			case ESampleCount::e64: return VK_SAMPLE_COUNT_64_BIT;
		}
		return VK_SAMPLE_COUNT_1_BIT;
	}

	constexpr auto get_vk_access_flags(EAccessFlags p_flags) -> VkAccessFlags2
	{
		VkAccessFlags2 out_flags{0ull};

		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eIndirectCommandRead, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eIndexRead, VK_ACCESS_2_INDEX_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eVertexAttributeRead, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eUniformRead, VK_ACCESS_2_UNIFORM_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eInputAttachmentRead, VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eShaderRead, VK_ACCESS_2_SHADER_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eShaderWrite, VK_ACCESS_2_SHADER_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eColorAttachmentRead, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eColorAttachmentWrite, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eDepthStencilAttachmentRead, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eDepthStencilAttachmentWrite, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eTransferRead, VK_ACCESS_2_TRANSFER_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eTransferWrite, VK_ACCESS_2_TRANSFER_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eHostRead, VK_ACCESS_2_HOST_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eHostWrite, VK_ACCESS_2_HOST_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eMemoryRead, VK_ACCESS_2_MEMORY_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eMemoryWrite, VK_ACCESS_2_MEMORY_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eShaderSampledRead, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eShaderStorageRead, VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eShaderStorageWrite, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eSamplerHeapRead, VK_ACCESS_2_SAMPLER_HEAP_READ_BIT_EXT);
		TST_GPU_FLAG_COMBINE(EAccessFlagBits::eResourceHeapRead, VK_ACCESS_2_RESOURCE_HEAP_READ_BIT_EXT);

		return out_flags;
	}

	constexpr auto get_vk_pipeline_stage_flags(EPipelineStageFlags p_flags) -> VkPipelineStageFlags2
	{
		VkPipelineStageFlags2 out_flags{0ull};

		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eTopOfPipe, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eDrawIndirect, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eVertexInput, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eVertexShader, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eTessellationControlShader, VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eTessellationEvaluationShader, VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eGeometryShader, VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eFragmentShader, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eEarlyFragmentTests, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eLateFragmentTests, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eColorAttachmentOutput, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eComputeShader, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eAllTransfer, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eTransfer, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eBottomOfPipe, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eHost, VK_PIPELINE_STAGE_2_HOST_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eAllGraphics, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eAllCommands, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eCopy, VK_PIPELINE_STAGE_2_COPY_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eResolve, VK_PIPELINE_STAGE_2_RESOLVE_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eBlit, VK_PIPELINE_STAGE_2_BLIT_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eClear, VK_PIPELINE_STAGE_2_CLEAR_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eIndexInput, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eVertexAttributeInput, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::ePreRasterizationShaders, VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eTaskShader, VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT);
		TST_GPU_FLAG_COMBINE(EPipelineStageFlagBits::eMeshShader, VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT);

		return out_flags;
	}

	constexpr auto get_vk_nv_latency_marker(EFrameMarker p_marker) -> VkLatencyMarkerNV
	{
		switch (p_marker)
		{
			case EFrameMarker::eSimulationStart: return VK_LATENCY_MARKER_SIMULATION_START_NV;
			case EFrameMarker::eSimulationEnd: return VK_LATENCY_MARKER_SIMULATION_END_NV;
			case EFrameMarker::eRendersubmitStart: return VK_LATENCY_MARKER_RENDERSUBMIT_START_NV;
			case EFrameMarker::eRendersubmitEnd: return VK_LATENCY_MARKER_RENDERSUBMIT_END_NV;
			case EFrameMarker::ePresentStart: return VK_LATENCY_MARKER_PRESENT_START_NV;
			case EFrameMarker::ePresentEnd: return VK_LATENCY_MARKER_PRESENT_END_NV;
			case EFrameMarker::eInputSample: return VK_LATENCY_MARKER_INPUT_SAMPLE_NV;
			case EFrameMarker::eTriggerFlash: return VK_LATENCY_MARKER_TRIGGER_FLASH_NV;
			case EFrameMarker::eOutOfBandRendersubmitStart: return VK_LATENCY_MARKER_OUT_OF_BAND_RENDERSUBMIT_START_NV;
			case EFrameMarker::eOutOfBandRendersubmitEnd: return VK_LATENCY_MARKER_OUT_OF_BAND_RENDERSUBMIT_END_NV;
			case EFrameMarker::eOutOfBandPresentStart: return VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_START_NV;
			case EFrameMarker::eOutOfBandPresentEnd: return VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_END_NV;
		}

		return VK_LATENCY_MARKER_INPUT_SAMPLE_NV;
	}

	constexpr auto get_vk_index_type(EIndexType p_index_type) -> VkIndexType
	{
		switch (p_index_type)
		{
			case EIndexType::eUint8: return VK_INDEX_TYPE_UINT8;
			case EIndexType::eUint16: return VK_INDEX_TYPE_UINT16;
			case EIndexType::eUint32: return VK_INDEX_TYPE_UINT32;
		}
		return VK_INDEX_TYPE_UINT32;
	}

	constexpr auto get_vk_primitive_topology(EPrimitiveTopology p_primitive_topology) -> VkPrimitiveTopology
	{
		switch (p_primitive_topology)
		{
			case EPrimitiveTopology::ePointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case EPrimitiveTopology::eLineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case EPrimitiveTopology::eLineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case EPrimitiveTopology::eTriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case EPrimitiveTopology::eTriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case EPrimitiveTopology::eTriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			case EPrimitiveTopology::eLineListWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
			case EPrimitiveTopology::eLineStripWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
			case EPrimitiveTopology::eTriangleListWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
			case EPrimitiveTopology::eTriangleStripWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
			case EPrimitiveTopology::ePatchList: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		}

		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	constexpr auto get_vk_polygon_mode(EPolygonMode p_polygon_mode) -> VkPolygonMode
	{
		switch (p_polygon_mode)
		{
			case EPolygonMode::eFill: return VK_POLYGON_MODE_FILL;
			case EPolygonMode::eLine: return VK_POLYGON_MODE_LINE;
			case EPolygonMode::ePoint: return VK_POLYGON_MODE_POINT;
		}
		return VK_POLYGON_MODE_FILL;
	}

	constexpr auto get_vk_cull_mode(ECullMode p_cull_mode) -> VkCullModeFlagBits
	{
		switch (p_cull_mode)
		{
			case ECullMode::eNone: return VK_CULL_MODE_NONE;;
			case ECullMode::eFront: return VK_CULL_MODE_FRONT_BIT;;
			case ECullMode::eBack: return VK_CULL_MODE_BACK_BIT;;
			case ECullMode::eFrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;;
		}
		return VK_CULL_MODE_NONE;
	}

	constexpr auto get_vk_compare_op(ECompareOp p_compare_op) -> VkCompareOp
	{
		switch (p_compare_op)
		{
			case ECompareOp::eNever: return VK_COMPARE_OP_NEVER;
			case ECompareOp::eLess: return VK_COMPARE_OP_LESS;
			case ECompareOp::eEqual: return VK_COMPARE_OP_EQUAL;
			case ECompareOp::eLessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case ECompareOp::eGreater: return VK_COMPARE_OP_GREATER;
			case ECompareOp::eNotEqual: return VK_COMPARE_OP_NOT_EQUAL;
			case ECompareOp::eGreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case ECompareOp::eAlways: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_NEVER;
	}

	constexpr auto get_vk_colour_blend_factor(EBlendFactor p_factor) -> VkBlendFactor
	{
		switch (p_factor)
		{
			case EBlendFactor::eZero: return VK_BLEND_FACTOR_ZERO;
			case EBlendFactor::eOne: return VK_BLEND_FACTOR_ONE;
			case EBlendFactor::eSrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
			case EBlendFactor::eOneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			case EBlendFactor::eDstColor: return VK_BLEND_FACTOR_DST_COLOR;
			case EBlendFactor::eOneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			case EBlendFactor::eSrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
			case EBlendFactor::eOneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case EBlendFactor::eDstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
			case EBlendFactor::eOneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			case EBlendFactor::eConstantColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
			case EBlendFactor::eOneMinusConstantColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			case EBlendFactor::eConstantAlpha: return VK_BLEND_FACTOR_CONSTANT_COLOR;
			case EBlendFactor::eOneMinusConstantAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
			case EBlendFactor::eSrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			case EBlendFactor::eSrc1Color: return VK_BLEND_FACTOR_SRC1_COLOR;
			case EBlendFactor::eOneMinusSrc1Color: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
			case EBlendFactor::eSrc1Alpha: return VK_BLEND_FACTOR_SRC1_ALPHA;
			case EBlendFactor::eOneMinusSrc1Alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		}
		return VK_BLEND_FACTOR_ZERO;
	}

	constexpr auto get_vk_colour_blend_op(EBlendOp p_op) -> VkBlendOp
	{
		switch (p_op)
		{
			case EBlendOp::eAdd: return VK_BLEND_OP_ADD;
			case EBlendOp::eSubtract: return VK_BLEND_OP_SUBTRACT;
			case EBlendOp::eReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
			case EBlendOp::eMin: return VK_BLEND_OP_MIN;
			case EBlendOp::eMax: return VK_BLEND_OP_MAX;
		}
		return VK_BLEND_OP_ADD;
	}

	constexpr auto get_vk_colour_component_flags(const EColourComponentFlags p_flags) -> VkColorComponentFlags
	{
		VkColorComponentFlags out_flags{0u};

		TST_GPU_FLAG_COMBINE(EColourComponentFlagBits::eR, VK_COLOR_COMPONENT_R_BIT);
		TST_GPU_FLAG_COMBINE(EColourComponentFlagBits::eG, VK_COLOR_COMPONENT_G_BIT);
		TST_GPU_FLAG_COMBINE(EColourComponentFlagBits::eB, VK_COLOR_COMPONENT_B_BIT);
		TST_GPU_FLAG_COMBINE(EColourComponentFlagBits::eA, VK_COLOR_COMPONENT_A_BIT);

		return out_flags;
	}

	constexpr auto get_vk_image_view_type(ETextureType p_type) -> VkImageViewType
	{
		switch (p_type)
		{
			case ETextureType::e1D: return VK_IMAGE_VIEW_TYPE_1D;
			case ETextureType::e2D: return VK_IMAGE_VIEW_TYPE_2D;
			case ETextureType::e3D: return VK_IMAGE_VIEW_TYPE_3D;
			case ETextureType::eCube: return VK_IMAGE_VIEW_TYPE_CUBE;
		}
		return VK_IMAGE_VIEW_TYPE_2D;
	}

	constexpr auto get_vk_image_type(ETextureType p_type) -> VkImageType
	{
		switch (p_type)
		{
			case ETextureType::e1D: return VK_IMAGE_TYPE_1D;
			case ETextureType::e2D: return VK_IMAGE_TYPE_2D;
			case ETextureType::e3D: return VK_IMAGE_TYPE_3D;
			case ETextureType::eCube: return VK_IMAGE_TYPE_2D;
		}
		return VK_IMAGE_TYPE_2D;
	}

	constexpr auto get_vk_address_mode(ESamplerAddressMode p_address_mode) -> VkSamplerAddressMode
	{
		switch (p_address_mode)
		{
			case ESamplerAddressMode::eRepeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case ESamplerAddressMode::eMirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case ESamplerAddressMode::eClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case ESamplerAddressMode::eClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	constexpr auto get_vk_buffer_usage(EBufferUsageFlags p_flags) -> VkBufferUsageFlags
	{
		VkBufferUsageFlags out_flags{0u};

		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eTransferSrc, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eTransferDst, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eIndirectBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eUniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		TST_GPU_FLAG_COMBINE(EBufferUsageFlagBits::eStorageBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

		return out_flags;
	}

	constexpr auto get_vk_image_usage(ETextureUsageFlags p_flags) -> VkImageUsageFlags
	{
		VkImageUsageFlags out_flags{0u};

		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eTransferSrc, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eTransferDst, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eSampled, VK_IMAGE_USAGE_SAMPLED_BIT);
		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eStorage, VK_IMAGE_USAGE_STORAGE_BIT);
		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eColourAttachment, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eDepthStencilAttachment, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		TST_GPU_FLAG_COMBINE(ETextureUsageFlagBits::eTransient, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);

		return out_flags;
	}

	#undef TST_GPU_FLAG_COMBINE

	auto gpu_ctx_create(const GPUContextDesc &p_desc) -> GPUContext *
	{
		GPUContext *ctx{new GPUContext{}};

		ctx->enableDebugInfo                 = p_desc.enableDebugInfo;
		ctx->usingSwapchain                  = p_desc.usingSwapchain;
		ctx->maxConcurrentSwapchainWorkloads = p_desc.maxConcurrentSwapchainWorkloads;

		for (uint32 i{0u}; i < TST_LIB_ENUM_VAL(EQueueType::eMax); ++i)
			mtx_init(&ctx->queueMutexes[i], mtx_plain);
		mtx_init(&ctx->freeCommandListMutex, mtx_plain);
		mtx_init(&ctx->undefinedTexturesMutex, mtx_plain);

		volkInitialize(); // Load vkGetInstanceProcAddress from the dll

		#pragma region instance
		{
			Vector<CString> required_extensions;

			if (p_desc.usingSwapchain)
			{
				required_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
				required_extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}
			if (p_desc.enableDebugInfo)
				required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			uint32 instance_extension_property_count;
			vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, nullptr);
			if (instance_extension_property_count == 0u)
			{
				TST_PERMA_ASSERT_MSG(false, "No Vulkan instance extensions were found!");
			}

			Vector<VkExtensionProperties> instance_extension_properties(instance_extension_property_count);
			vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_property_count, instance_extension_properties.data());

			// Make sure that all the required extensions are present in the extension_props vector
			const auto unsupported_extension{
				std::ranges::find_if(required_extensions, [&instance_extension_properties](CString p_extension)
				{
					// returns true if none of the extensions are present (the strcmp would always evaluate to false)
					return std::ranges::none_of(instance_extension_properties, [p_extension](const VkExtensionProperties &p_properties)
					{
						return std::strcmp(p_properties.extensionName, p_extension) == 0;
					});
				})
			};

			if (unsupported_extension != required_extensions.end())
			{
				// We can't continue without the required extensions, so terminate the program here
				std::printf("Required extension \"%s\" is not supported", *unsupported_extension);
				TST_PERMA_ASSERT(false);
			}

			Vector<CString> required_validation_layers;
			if (p_desc.enableDebugInfo)
			{
				required_validation_layers.emplace_back("VK_LAYER_KHRONOS_validation");

				uint32 instance_layer_properties_count;
				vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, nullptr);

				Vector<VkLayerProperties> instance_layer_properties(instance_layer_properties_count);
				vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, instance_layer_properties.data());

				// Finds any layer in required_validation_layers, such that it is also not present in the actual layer_props vector
				const auto unsupported_layer_it{
					std::ranges::find_if(required_validation_layers, [&instance_layer_properties](CString p_layer)
					{
						return std::ranges::none_of(instance_layer_properties, [p_layer](const VkLayerProperties &p_properties)
						{
							return std::strcmp(p_properties.layerName, p_layer) == 0;
						});
					})
				};

				// Check to see if there are any unsupported layers
				if (unsupported_layer_it != required_validation_layers.end())
				{
					// We can't continue without the required validation layers, so terminate the program here
					std::printf("Found unsupported validation layer: %s", *unsupported_layer_it);
					TST_PERMA_ASSERT(false);
				}
			}

			VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
			debug_messenger_create_info.sType = STYPE(DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
			debug_messenger_create_info.pNext = nullptr;
			if (p_desc.enableDebugInfo)
			{
				constexpr VkDebugUtilsMessageSeverityFlagsEXT severity_flags{
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				};
				constexpr VkDebugUtilsMessageTypeFlagsEXT message_type_flags{
					VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
				};
				debug_messenger_create_info.messageSeverity = severity_flags;
				debug_messenger_create_info.messageType     = message_type_flags;
				debug_messenger_create_info.pfnUserCallback = &vk_debug_callback;
			}

			VkApplicationInfo application_info{};
			application_info.sType              = STYPE(APPLICATION_INFO);
			application_info.pNext              = nullptr;
			application_info.pApplicationName   = "Toaster_Application";
			application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			application_info.pEngineName        = "Toaster";
			application_info.engineVersion      = VK_MAKE_VERSION(3, 0, 0);
			application_info.apiVersion         = VK_API_VERSION_1_4;

			VkInstanceCreateInfo instance_create_info{};
			instance_create_info.sType                   = STYPE(INSTANCE_CREATE_INFO);
			instance_create_info.pNext                   = nullptr;
			instance_create_info.pApplicationInfo        = &application_info;
			instance_create_info.enabledExtensionCount   = required_extensions.size();
			instance_create_info.ppEnabledExtensionNames = required_extensions.data();

			if (p_desc.enableDebugInfo)
			{
				instance_create_info.enabledLayerCount   = required_validation_layers.size();
				instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
				instance_create_info.pNext               = &debug_messenger_create_info;
			}

			TST_GPU_VK_CHECK_RESULT_MSG(vkCreateInstance(&instance_create_info, nullptr, &ctx->vulkanInstance), "Failed to create Vulkan instance");

			volkLoadInstance(ctx->vulkanInstance); // Load all the instance-level Vulkan functions
		}
		#pragma endregion

		Vector<CString> required_device_extensions{
			VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
			VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
			VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME,
			VK_EXT_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_9_EXTENSION_NAME,
			VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME,
			VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,
			VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
			VK_NV_LOW_LATENCY_2_EXTENSION_NAME,
		};
		if (p_desc.usingSwapchain)
			required_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		#pragma region physical device
		{
			uint32 physical_device_count;
			vkEnumeratePhysicalDevices(ctx->vulkanInstance, &physical_device_count, nullptr);

			if (physical_device_count == 0u)
			{
				// If your gpu does not have Vulkan support, we can't use Vulkan
				TST_PERMA_ASSERT_MSG(false, "Failed to find physical devices with Vulkan support");
			}

			Vector<VkPhysicalDevice> physical_devices(physical_device_count);
			vkEnumeratePhysicalDevices(ctx->vulkanInstance, &physical_device_count, physical_devices.data());

			const auto device_it{
				std::ranges::find_if(physical_devices, [&required_device_extensions](const VkPhysicalDevice p_device)
				{
					return is_device_suitable(p_device, required_device_extensions);
				})
			};

			if (device_it == physical_devices.end())
			{
				TST_PERMA_ASSERT_MSG(false, "Failed to find suitable physical device");
			}

			ctx->physicalDevice = *device_it;

			ctx->descriptorHeapProperties.sType = STYPE(PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT);
			ctx->descriptorHeapProperties.pNext = nullptr;
			ctx->deviceProperties.sType         = STYPE(PHYSICAL_DEVICE_PROPERTIES_2);
			ctx->deviceProperties.pNext         = &ctx->descriptorHeapProperties;
			vkGetPhysicalDeviceProperties2(ctx->physicalDevice, &ctx->deviceProperties);
		}

		#pragma endregion

		#pragma region logical device

		ctx->queueFamilyIndices = select_queue_family_indices(ctx->physicalDevice, p_desc.usingSwapchain);

		if (ctx->queueFamilyIndices.graphics == UINT32_MAX)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to find a graphics queue family");
		}

		if (ctx->queueFamilyIndices.transfer == UINT32_MAX) // Graphics queues are guaranteed to support transfer as well
			ctx->queueFamilyIndices.transfer = ctx->queueFamilyIndices.graphics;

		constexpr float32 default_queue_priority{1.0f};

		Array<VkDeviceQueueCreateInfo, 2u> queue_create_infos;

		VkDeviceQueueCreateInfo &graphics_queue_create_info{queue_create_infos[0u]};
		graphics_queue_create_info.sType            = STYPE(DEVICE_QUEUE_CREATE_INFO);
		graphics_queue_create_info.pNext            = nullptr;
		graphics_queue_create_info.flags            = 0u;
		graphics_queue_create_info.queueCount       = 1u;
		graphics_queue_create_info.pQueuePriorities = &default_queue_priority;
		graphics_queue_create_info.queueFamilyIndex = ctx->queueFamilyIndices.graphics;

		if (ctx->queueFamilyIndices.transfer != ctx->queueFamilyIndices.graphics)
		{
			VkDeviceQueueCreateInfo &transfer_queue_create_info{queue_create_infos[1u]};
			transfer_queue_create_info.sType            = STYPE(DEVICE_QUEUE_CREATE_INFO);
			transfer_queue_create_info.pNext            = nullptr;
			transfer_queue_create_info.flags            = 0u;
			transfer_queue_create_info.queueCount       = 1u;
			transfer_queue_create_info.pQueuePriorities = &default_queue_priority;
			transfer_queue_create_info.queueFamilyIndex = ctx->queueFamilyIndices.transfer;
		}

		// This removes the need to transfer queue ownership for buffers. Not optimally tiles images though
		VkPhysicalDeviceMaintenance9FeaturesKHR maintenance_9_features{};
		maintenance_9_features.sType        = STYPE(PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR);
		maintenance_9_features.pNext        = nullptr;
		maintenance_9_features.maintenance9 = true;

		// Apparently colour write enable is it's own extension for some reason
		VkPhysicalDeviceColorWriteEnableFeaturesEXT colour_write_enable_features{};
		colour_write_enable_features.sType            = STYPE(PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT);
		colour_write_enable_features.pNext            = &maintenance_9_features;
		colour_write_enable_features.colorWriteEnable = true;

		// Allows for optimised OIT by using critical sections inside of the fragment shader.
		VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragment_shader_interlock_features{};
		fragment_shader_interlock_features.sType                        = STYPE(PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT);
		fragment_shader_interlock_features.pNext                        = &colour_write_enable_features;
		fragment_shader_interlock_features.fragmentShaderPixelInterlock = true;

		// Vertex shader + Compute shader + Geometry shader + Tess Control shader + Tess Eval shader = Holy pipeline.
		VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features{};
		mesh_shader_features.sType      = STYPE(PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT);
		mesh_shader_features.pNext      = &fragment_shader_interlock_features;
		mesh_shader_features.meshShader = true;
		mesh_shader_features.taskShader = true;

		// Essentially removes the hassle of transitioning image layouts by allowing for images to be in the GENERAL layout.
		// The driver will automatically know the usage and optimise from there. The exception is swapchain images, as they are part of an extension.
		VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unified_image_layout_features{};
		unified_image_layout_features.sType               = STYPE(PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR);
		unified_image_layout_features.pNext               = &mesh_shader_features;
		unified_image_layout_features.unifiedImageLayouts = true;

		// Allows for descriptor heaps to actually be declared inside of shaders
		VkPhysicalDeviceShaderUntypedPointersFeaturesKHR shader_untyped_pointer_features{};
		shader_untyped_pointer_features.sType                 = STYPE(PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR);
		shader_untyped_pointer_features.pNext                 = &unified_image_layout_features;
		shader_untyped_pointer_features.shaderUntypedPointers = true;

		// Descriptor heaps allow for the complete bypassing of all VkDescriptorXXX state. Instead, using indices into global "resource" and "sampler" heaps.
		VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptor_heap_features{};
		descriptor_heap_features.sType          = STYPE(PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT);
		descriptor_heap_features.pNext          = &shader_untyped_pointer_features;
		descriptor_heap_features.descriptorHeap = true;

		// Using shader objects allows for the complete bypassing of any VkPipeline objects. Meaning all state has to be set dynamically
		VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_features{};
		shader_object_features.sType        = STYPE(PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT);
		shader_object_features.pNext        = &descriptor_heap_features;
		shader_object_features.shaderObject = true;

		VkPhysicalDeviceVulkan11Features vulkan_11_features{};
		vulkan_11_features.sType                              = STYPE(PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
		vulkan_11_features.pNext                              = &shader_object_features;
		vulkan_11_features.storageBuffer16BitAccess           = true; // Allows storage buffers to have 16-bit members
		vulkan_11_features.uniformAndStorageBuffer16BitAccess = true; // Allows uniform buffers to have 16-bit members
		vulkan_11_features.storagePushConstant16              = true; // Allows push constants to have 16-bit members
		vulkan_11_features.shaderDrawParameters               = true; // Fixes a validation error.

		VkPhysicalDeviceVulkan12Features vulkan_12_features{};
		vulkan_12_features.sType                             = STYPE(PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
		vulkan_12_features.pNext                             = &vulkan_11_features;
		vulkan_12_features.drawIndirectCount                 = true; // Used in MDI
		vulkan_12_features.storageBuffer8BitAccess           = true; // Allows storage buffers to have 8-bit members
		vulkan_12_features.uniformAndStorageBuffer8BitAccess = true; // Allows uniform buffers to have 8-bit members
		vulkan_12_features.storagePushConstant8              = true; // Allows push constants to have 8-bit members
		vulkan_12_features.bufferDeviceAddress               = true; // The best feature
		vulkan_12_features.scalarBlockLayout                 = true; // Very useful to avoid having to use std140 / std430 alignment rules
		vulkan_12_features.shaderInt8                        = true; // uint8_t. Used in meshlet triangle buffers
		vulkan_12_features.timelineSemaphore                 = true; // SemaphoreHandles are just timeline semaphores

		VkPhysicalDeviceVulkan13Features vulkan_13_features{};
		vulkan_13_features.sType            = STYPE(PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
		vulkan_13_features.pNext            = &vulkan_12_features;
		vulkan_13_features.synchronization2 = true; // Better synchronization commands
		vulkan_13_features.dynamicRendering = true; // Much better than render passes
		vulkan_13_features.maintenance4     = true; // Fixes this: OpExecutionMode LocalSizeId is used but maintenance4 feature was not enabled.

		VkPhysicalDeviceVulkan14Features vulkan_14_features{};
		vulkan_14_features.sType          = STYPE(PHYSICAL_DEVICE_VULKAN_1_4_FEATURES);
		vulkan_14_features.pNext          = &vulkan_13_features;
		vulkan_14_features.indexTypeUint8 = true; // Allows index buffers to use 8-bit unsigned integers. used in meshlet triangle buffers

		VkPhysicalDeviceFeatures2 features_2{};
		features_2.sType = STYPE(PHYSICAL_DEVICE_FEATURES_2);
		features_2.pNext = &vulkan_14_features;

		VkPhysicalDeviceFeatures &core_features{features_2.features};
		core_features.fullDrawIndexUint32               = true; // This means that index buffers can utilise the full range of 32 bit values (2^32 - 1)
		core_features.multiDrawIndirect                 = true; // MDI is very useful
		core_features.drawIndirectFirstInstance         = true; // The first instance member allows you to put custom instance ids in the draw command
		core_features.fillModeNonSolid                  = true; // I want to support wireframe line rendering
		core_features.samplerAnisotropy                 = true; // Obviously I want to be able to use anisotropic filtering
		core_features.fragmentStoresAndAtomics          = true; // For interlock OIT, you need to store data in a fragment shader SSBO
		core_features.shaderStorageImageExtendedFormats = true; // Docs says enabling this has no effect...
		core_features.shaderInt64                       = true; // int64_t
		core_features.shaderInt16                       = true; // int16_t

		VkDeviceCreateInfo logical_device_create_info{};
		logical_device_create_info.sType                   = STYPE(DEVICE_CREATE_INFO);
		logical_device_create_info.flags                   = 0u;
		logical_device_create_info.queueCreateInfoCount    = queue_create_infos.size();
		logical_device_create_info.pQueueCreateInfos       = queue_create_infos.data();
		logical_device_create_info.enabledExtensionCount   = required_device_extensions.size();
		logical_device_create_info.ppEnabledExtensionNames = required_device_extensions.data();
		logical_device_create_info.pNext                   = &features_2;

		VkResult result{vkCreateDevice(ctx->physicalDevice, &logical_device_create_info, nullptr, &ctx->logicalDevice)};
		TST_PERMA_ASSERT(result == VK_SUCCESS);
		// TST_GPU_VK_CHECK_RESULT_MSG(vkCreateDevice(ctx->physicalDevice, &logical_device_create_info, nullptr, &ctx->logicalDevice), "Failed to create logical device");

		// Graphics queue
		vkGetDeviceQueue(ctx->logicalDevice, ctx->queueFamilyIndices.graphics, 0u, &ctx->queues[TST_LIB_ENUM_VAL(EQueueType::eGraphics)]);

		// Not the compute queue
		ctx->queues[TST_LIB_ENUM_VAL(EQueueType::eCompute)] = nullptr; // TODO: More queues

		// Transfer queue
		uint32 transfer_queue_index{(ctx->queueFamilyIndices.transfer == ctx->queueFamilyIndices.graphics) ? 1u : 0u};
		vkGetDeviceQueue(ctx->logicalDevice, ctx->queueFamilyIndices.transfer, transfer_queue_index, &ctx->queues[TST_LIB_ENUM_VAL(EQueueType::eTransfer)]);

		for (uint32 queue_index{0u}; queue_index < TST_LIB_ENUM_VAL(EQueueType::eMax); ++queue_index)
		{
			VkCommandPoolCreateInfo command_pool_create_info{};
			command_pool_create_info.sType            = STYPE(COMMAND_POOL_CREATE_INFO);
			command_pool_create_info.pNext            = nullptr;
			command_pool_create_info.queueFamilyIndex = ctx->queueFamilyIndices.get(static_cast<EQueueType>(queue_index));
			command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			TST_GPU_VK_CHECK_RESULT_MSG(vkCreateCommandPool(ctx->logicalDevice, &command_pool_create_info, nullptr, &ctx->transientPools[queue_index]),
										"Failed to create transient command pool");
		}

		volkLoadDevice(ctx->logicalDevice); // Load all the device-level Vulkan functions

		#pragma endregion

		#pragma region allocator

		VmaVulkanFunctions vma_vulkan_functions{};
		vma_vulkan_functions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
		vma_vulkan_functions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
		vma_vulkan_functions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
		vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
		vma_vulkan_functions.vkAllocateMemory                        = vkAllocateMemory;
		vma_vulkan_functions.vkFreeMemory                            = vkFreeMemory;
		vma_vulkan_functions.vkMapMemory                             = vkMapMemory;
		vma_vulkan_functions.vkUnmapMemory                           = vkUnmapMemory;
		vma_vulkan_functions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
		vma_vulkan_functions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
		vma_vulkan_functions.vkBindBufferMemory                      = vkBindBufferMemory;
		vma_vulkan_functions.vkBindImageMemory                       = vkBindImageMemory;
		vma_vulkan_functions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
		vma_vulkan_functions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
		vma_vulkan_functions.vkCreateBuffer                          = vkCreateBuffer;
		vma_vulkan_functions.vkDestroyBuffer                         = vkDestroyBuffer;
		vma_vulkan_functions.vkCreateImage                           = vkCreateImage;
		vma_vulkan_functions.vkDestroyImage                          = vkDestroyImage;
		vma_vulkan_functions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
		vma_vulkan_functions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR;
		vma_vulkan_functions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR;
		vma_vulkan_functions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR;
		vma_vulkan_functions.vkBindImageMemory2KHR                   = vkBindImageMemory2KHR;
		vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
		vma_vulkan_functions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
		vma_vulkan_functions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;
		vma_vulkan_functions.vkGetMemoryWin32HandleKHR               = vkGetMemoryWin32HandleKHR;
		vma_vulkan_functions.vkGetPhysicalDeviceProperties2KHR       = vkGetPhysicalDeviceProperties2KHR;

		VmaAllocatorCreateInfo allocator_create_info{};
		allocator_create_info.instance         = ctx->vulkanInstance;
		allocator_create_info.physicalDevice   = ctx->physicalDevice;
		allocator_create_info.device           = ctx->logicalDevice;
		allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_4;
		allocator_create_info.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocator_create_info.pVulkanFunctions = &vma_vulkan_functions;

		vmaCreateAllocator(&allocator_create_info, &ctx->allocator);

		#pragma endregion

		#pragma region destroy callbacks

		ctx->commandLists.setDestructorUserData(ctx);
		ctx->commandLists.setDestructorFn(+[](CommandList *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vkDestroyCommandPool(ctx->logicalDevice, p_data->pool, nullptr);

			p_data->pool = nullptr;
			p_data->cmd  = nullptr;
		});

		ctx->resourceHeaps.setDestructorUserData(ctx);
		ctx->resourceHeaps.setDestructorFn(+[](ResourceDescriptorHeap *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vmaDestroyBuffer(ctx->allocator, p_data->heapBuffer, p_data->heapAllocation);
			mtx_destroy(&p_data->imageSlotMutex);
			mtx_destroy(&p_data->bufferSlotMutex);

			p_data->heapBuffer       = nullptr;
			p_data->heapAllocation   = nullptr;
			p_data->mappedHeapMemory = nullptr;
		});

		ctx->samplerHeaps.setDestructorUserData(ctx);
		ctx->samplerHeaps.setDestructorFn(+[](SamplerDescriptorHeap *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vmaDestroyBuffer(ctx->allocator, p_data->heapBuffer, p_data->heapAllocation);
			mtx_destroy(&p_data->samplerSlotMutex);

			p_data->heapBuffer       = nullptr;
			p_data->heapAllocation   = nullptr;
			p_data->mappedHeapMemory = nullptr;
		});

		ctx->semaphores.setDestructorUserData(ctx);
		ctx->semaphores.setDestructorFn(+[](Semaphore *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vkDestroySemaphore(ctx->logicalDevice, p_data->semaphore, nullptr);

			p_data->semaphore = nullptr;
		});

		ctx->buffers.setDestructorUserData(ctx);
		ctx->buffers.setDestructorFn(+[](Buffer *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vmaDestroyBuffer(ctx->allocator, p_data->buffer, p_data->allocation);

			p_data->buffer     = nullptr;
			p_data->allocation = nullptr;
			p_data->mapped     = nullptr;
		});

		ctx->textures.setDestructorUserData(ctx);
		ctx->textures.setDestructorFn(+[](Texture *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			if (p_data->imageView)
				vkDestroyImageView(ctx->logicalDevice, p_data->imageView, nullptr);

			if (!p_data->isSwapchainImage)
				vmaDestroyImage(ctx->allocator, p_data->image, p_data->allocation);

			p_data->image      = nullptr;
			p_data->allocation = nullptr;
			p_data->imageView  = nullptr;
		});

		ctx->surfaces.setDestructorUserData(ctx);
		ctx->surfaces.setDestructorFn(+[](Surface *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vkDestroySurfaceKHR(ctx->vulkanInstance, p_data->surface, nullptr);

			p_data->surface = nullptr;
		});

		ctx->swapchains.setDestructorUserData(ctx);
		ctx->swapchains.setDestructorFn(+[](Swapchain *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			for (auto &attachment: p_data->attachments)
				ctx->textures.destroy(attachment);

			for (auto &semaphore: p_data->imageAvailableSemaphores)
				vkDestroySemaphore(ctx->logicalDevice, semaphore, nullptr);

			for (auto &semaphore: p_data->renderFinishedSemaphores)
				vkDestroySemaphore(ctx->logicalDevice, semaphore, nullptr);

			vkDestroySwapchainKHR(ctx->logicalDevice, p_data->swapchain, nullptr);

			semaphore_destroy(ctx, p_data->latencySemaphore);

			p_data->attachments.clear();
			p_data->imageAvailableSemaphores.clear();
			p_data->swapchain        = nullptr;
			p_data->latencySemaphore = nullptr;
		});

		ctx->shaders.setDestructorUserData(ctx);
		ctx->shaders.setDestructorFn(+[](Shader *p_data, void *p_ctx) mutable noexcept -> void
		{
			const auto ctx{static_cast<GPUContext *>(p_ctx)};

			vkDestroyShaderEXT(ctx->logicalDevice, p_data->shader, nullptr);

			p_data->shader = nullptr;
		});

		#pragma endregion

		return ctx;
	}

	auto gpu_ctx_destroy(GPUContext *p_ctx) -> void
	{
		vkDeviceWaitIdle(p_ctx->logicalDevice);

		p_ctx->resourceHeaps.clear();
		p_ctx->samplerHeaps.clear();
		p_ctx->textures.clear();
		p_ctx->buffers.clear();
		p_ctx->shaders.clear();
		p_ctx->swapchains.clear();
		p_ctx->surfaces.clear();
		p_ctx->semaphores.clear();
		p_ctx->commandLists.clear();

		for (uint32 queue_index{0u}; queue_index < TST_LIB_ENUM_VAL(EQueueType::eMax); ++queue_index)
			vkDestroyCommandPool(p_ctx->logicalDevice, p_ctx->transientPools[queue_index], nullptr);

		vmaDestroyAllocator(p_ctx->allocator);
		vkDestroyDevice(p_ctx->logicalDevice, nullptr);
		vkDestroyInstance(p_ctx->vulkanInstance, nullptr);

		for (uint32 i{0u}; i < TST_LIB_ENUM_VAL(EQueueType::eMax); ++i)
			mtx_destroy(&p_ctx->queueMutexes[i]);
		mtx_destroy(&p_ctx->freeCommandListMutex);
		mtx_destroy(&p_ctx->undefinedTexturesMutex);

		delete p_ctx;
	}

	auto gpu_ctx_print_allocation_statistics(const GPUContext *p_ctx) -> void
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(p_ctx->allocator, &stats);
		std::println("Allocation byte size: {}", ByteSize{stats.total.statistics.allocationBytes});
		std::println("VMA allocated from driver: {}", ByteSize{stats.total.statistics.blockBytes});
	}

	auto gpu_ctx_dump_allocation_statistics_to_json(const GPUContext *p_ctx, StringView p_path) -> void
	{
		char *stats_string;
		vmaBuildStatsString(p_ctx->allocator, &stats_string, true);

		FILE *out_file{fopen(p_path.data(), "w")};
		fwrite(stats_string, sizeof(char), strlen(stats_string), out_file);
		fclose(out_file);

		vmaFreeStatsString(p_ctx->allocator, stats_string);
	}

	auto wait_idle(const GPUContext *p_ctx) -> void
	{
		vkDeviceWaitIdle(p_ctx->logicalDevice);
	}

	auto wait_queue_idle(const GPUContext *p_ctx, EQueueType p_queue_type) -> void
	{
		const VkQueue queue{p_ctx->queues[static_cast<uint32>(p_queue_type)]};
		vkQueueWaitIdle(queue);
	}

	auto get_pending_image_memory_barriers(GPUContext *p_ctx, EQueueType p_queue_type) -> VkCommandBuffer
	{
		mtx_lock(&p_ctx->undefinedTexturesMutex);

		if (p_ctx->undefinedTextures.empty() || p_queue_type == EQueueType::eTransfer) // Transfer queues cannot perform layout transitions
		{
			mtx_unlock(&p_ctx->undefinedTexturesMutex);
			return nullptr;
		}

		Vector<VkImageMemoryBarrier2> image_memory_barriers(p_ctx->undefinedTextures.size());

		// unordered sets can't be indexed normally
		uint32 i{0u};
		for (const auto undef_tex: p_ctx->undefinedTextures)
		{
			const Texture &    texture{p_ctx->textures[undef_tex]};
			const TextureDesc &desc{texture.desc};

			VkImageMemoryBarrier2 &barrier{image_memory_barriers[i]};
			barrier.sType               = STYPE(IMAGE_MEMORY_BARRIER_2);
			barrier.pNext               = nullptr;
			barrier.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			barrier.srcAccessMask       = VK_ACCESS_2_NONE;
			barrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			barrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT; // Covers every access flag basically
			barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image               = texture.image;

			VkImageSubresourceRange &subresource_range{barrier.subresourceRange};
			subresource_range.aspectMask     = get_vk_image_aspect_mask(desc.format);
			subresource_range.baseMipLevel   = 0u;
			subresource_range.levelCount     = desc.mipCount;
			subresource_range.baseArrayLayer = 0u;
			subresource_range.layerCount     = desc.layerCount;

			++i;
		}

		// I forgot to clear this. I only realised when my RAM maxed out.
		p_ctx->undefinedTextures.clear();

		mtx_unlock(&p_ctx->undefinedTexturesMutex);

		VkDependencyInfo dependency_info{};
		dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
		dependency_info.pNext                    = nullptr;
		dependency_info.imageMemoryBarrierCount  = image_memory_barriers.size();
		dependency_info.pImageMemoryBarriers     = image_memory_barriers.data();
		dependency_info.bufferMemoryBarrierCount = 0u;
		dependency_info.memoryBarrierCount       = 0u;

		// This command buffer will only live for this function call. So it is allocated using one of the transient pools
		const VkCommandPool command_pool{p_ctx->transientPools[static_cast<uint32>(p_queue_type)]};

		VkCommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info.sType              = STYPE(COMMAND_BUFFER_ALLOCATE_INFO);
		command_buffer_alloc_info.pNext              = nullptr;
		command_buffer_alloc_info.commandPool        = command_pool;
		command_buffer_alloc_info.commandBufferCount = 1u;
		command_buffer_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer command_buffer;
		TST_GPU_VK_CHECK_RESULT_MSG_DEBUG(vkAllocateCommandBuffers(p_ctx->logicalDevice, &command_buffer_alloc_info, &command_buffer),
										  "Failed to allocate transient command buffer");

		if (p_ctx->enableDebugInfo)
		{
			VkDebugUtilsObjectNameInfoEXT debug_object_name_info{};
			debug_object_name_info.sType        = STYPE(DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
			debug_object_name_info.pNext        = nullptr;
			debug_object_name_info.objectHandle = reinterpret_cast<uint64>(command_buffer);
			debug_object_name_info.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
			debug_object_name_info.pObjectName  = "Transient image memory barrier command buffer";
			vkSetDebugUtilsObjectNameEXT(p_ctx->logicalDevice, &debug_object_name_info);
		}

		VkCommandBufferBeginInfo command_buffer_begin_info{};
		command_buffer_begin_info.sType            = STYPE(COMMAND_BUFFER_BEGIN_INFO);
		command_buffer_begin_info.pNext            = nullptr;
		command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		command_buffer_begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
		vkCmdPipelineBarrier2(command_buffer, &dependency_info);
		vkEndCommandBuffer(command_buffer);

		return command_buffer;
	}

	auto cmd_list_get_or_create(GPUContext *p_ctx, EQueueType p_queue_type, bool p_secondary) -> CommandListHandle
	{
		mtx_lock(&p_ctx->freeCommandListMutex);

		auto &free_lists{p_secondary ? p_ctx->freeSecondaryLists[static_cast<uint32>(p_queue_type)] : p_ctx->freeCommandLists[static_cast<uint32>(p_queue_type)]};
		if (!free_lists.empty())
		{
			const CommandListHandle command_list_handle{free_lists.back()};
			free_lists.pop_back();

			mtx_unlock(&p_ctx->freeCommandListMutex);

			return command_list_handle;
		}

		mtx_unlock(&p_ctx->freeCommandListMutex);

		VkCommandPoolCreateInfo command_pool_create_info{};
		command_pool_create_info.sType            = STYPE(COMMAND_POOL_CREATE_INFO);
		command_pool_create_info.pNext            = nullptr;
		command_pool_create_info.queueFamilyIndex = p_ctx->queueFamilyIndices.get(p_queue_type);

		VkCommandPool command_pool;
		TST_GPU_VK_CHECK_RESULT_MSG_DEBUG(vkCreateCommandPool(p_ctx->logicalDevice, &command_pool_create_info, nullptr, &command_pool), "Failed to create command pool");

		VkCommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info.sType              = STYPE(COMMAND_BUFFER_ALLOCATE_INFO);
		command_buffer_alloc_info.pNext              = nullptr;
		command_buffer_alloc_info.commandPool        = command_pool;
		command_buffer_alloc_info.commandBufferCount = 1u;
		command_buffer_alloc_info.level              = p_secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer command_buffer;
		TST_GPU_VK_CHECK_RESULT_MSG_DEBUG(vkAllocateCommandBuffers(p_ctx->logicalDevice, &command_buffer_alloc_info, &command_buffer),
										  "Failed to allocate command buffer");

		if (p_ctx->enableDebugInfo)
		{
			VkDebugUtilsObjectNameInfoEXT debug_object_name_info{};
			debug_object_name_info.sType        = STYPE(DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
			debug_object_name_info.pNext        = nullptr;
			debug_object_name_info.objectHandle = reinterpret_cast<uint64>(command_buffer);
			debug_object_name_info.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
			debug_object_name_info.pObjectName  = to_string(p_queue_type);
			vkSetDebugUtilsObjectNameEXT(p_ctx->logicalDevice, &debug_object_name_info);
		}

		return p_ctx->commandLists.emplace(CommandList{command_buffer, command_pool, p_queue_type, false, p_secondary});
	}

	auto cmd_list_open(GPUContext *p_ctx, CommandListHandle p_cmd_list, const CommandListInheritanceInfo *p_inheritance_info) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		TST_ASSERT_MSG(!cmd.open, "Command list is already open");
		cmd.open = true;

		VkCommandBufferInheritanceDescriptorHeapInfoEXT descriptor_heap_inheritance_info{};
		descriptor_heap_inheritance_info.sType = STYPE(COMMAND_BUFFER_INHERITANCE_DESCRIPTOR_HEAP_INFO_EXT);
		descriptor_heap_inheritance_info.pNext = nullptr;

		if (cmd.secondary && p_inheritance_info)
		{
			const ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_inheritance_info->resourceHeap]};
			const SamplerDescriptorHeap & sampler_heap{p_ctx->samplerHeaps[p_inheritance_info->samplerHeap]};

			descriptor_heap_inheritance_info.pResourceHeapBindInfo = &resource_heap.bindInfo;
			descriptor_heap_inheritance_info.pSamplerHeapBindInfo  = &sampler_heap.bindInfo;
		}

		VkCommandBufferInheritanceRenderingInfo rendering_inheritance_info{};
		rendering_inheritance_info.sType = STYPE(COMMAND_BUFFER_INHERITANCE_RENDERING_INFO);
		rendering_inheritance_info.pNext = nullptr;

		Vector<VkFormat> colour_attachment_formats;
		if (cmd.secondary && p_inheritance_info)
		{
			colour_attachment_formats.resize(p_inheritance_info->colourAttachmentFormats.size());
			for (uint32 i{0u}; i < p_inheritance_info->colourAttachmentFormats.size(); ++i)
				colour_attachment_formats[i] = get_vk_format(p_inheritance_info->colourAttachmentFormats[i]);

			rendering_inheritance_info.colorAttachmentCount    = colour_attachment_formats.size();
			rendering_inheritance_info.pColorAttachmentFormats = colour_attachment_formats.data();
			rendering_inheritance_info.depthAttachmentFormat   = get_vk_format(p_inheritance_info->depthAttachmentFormat);
			rendering_inheritance_info.stencilAttachmentFormat = get_vk_format(p_inheritance_info->stencilAttachmentFormat);
			rendering_inheritance_info.rasterizationSamples    = get_vk_sample_count(p_inheritance_info->samples);
			rendering_inheritance_info.pNext                   = &descriptor_heap_inheritance_info;
		}

		VkCommandBufferInheritanceInfo inheritance_info{};
		inheritance_info.sType = STYPE(COMMAND_BUFFER_INHERITANCE_INFO);
		inheritance_info.pNext = nullptr;
		if (cmd.secondary && p_inheritance_info)
			inheritance_info.pNext = &rendering_inheritance_info;

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = STYPE(COMMAND_BUFFER_BEGIN_INFO);
		begin_info.pNext = nullptr;
		begin_info.flags = cmd.secondary ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (cmd.secondary && p_inheritance_info)
			begin_info.pInheritanceInfo = &inheritance_info;

		vkBeginCommandBuffer(cmd.cmd, &begin_info);
	}

	auto cmd_list_close(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		TST_ASSERT_MSG(cmd.open, "Command list must be open to close it");
		cmd.open = false;
		vkEndCommandBuffer(cmd.cmd);
	}

	auto cmd_list_free(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT_MSG(!cmd.open, "Command buffer must not be open when being freed");

		mtx_lock(&p_ctx->freeCommandListMutex);

		if (cmd.secondary)
			p_ctx->freeSecondaryLists[static_cast<uint32>(cmd.queueType)].push_back(p_cmd_list);
		else
			p_ctx->freeCommandLists[static_cast<uint32>(cmd.queueType)].push_back(p_cmd_list);

		mtx_unlock(&p_ctx->freeCommandListMutex);
	}

	auto cmd_list_reset(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		if (cmd.open)
			cmd.open = false;

		vkResetCommandPool(p_ctx->logicalDevice, cmd.pool, 0u);
	}

	// Accepts raw Vulkan semaphores
	static auto submit(GPUContext *                                 p_ctx, EQueueType p_queue_type, InitialiserList<const CommandListHandle> p_cmd_lists,
					   InitialiserList<const VkSemaphoreSubmitInfo> p_wait_semaphore_infos, InitialiserList<const VkSemaphoreSubmitInfo> p_signal_semaphore_infos) -> void
	{
		VkCommandBuffer image_memory_barrier_cmd{get_pending_image_memory_barriers(p_ctx, p_queue_type)};

		Vector<VkCommandBufferSubmitInfo> command_buffer_submit_infos;
		command_buffer_submit_infos.reserve(p_cmd_lists.size() + 1u); // Reserve for the image memory barrier
		for (uint64 i{0u}; i < p_cmd_lists.size(); ++i)
		{
			const CommandList &cmd{p_ctx->commandLists[p_cmd_lists[i]]};
			TST_ASSERT_MSG(!cmd.open, "Cannot submit a command buffer that is not open");
			TST_ASSERT_MSG(!cmd.secondary, "Cannot submit a secondary command buffer!");
			command_buffer_submit_infos.emplace_back(STYPE(COMMAND_BUFFER_SUBMIT_INFO), nullptr, cmd.cmd, 0u);
		}

		if (image_memory_barrier_cmd) // I need to make sure that the image memory barriers are submitted first so they can clear properly
		{
			command_buffer_submit_infos.insert(command_buffer_submit_infos.begin(), VkCommandBufferSubmitInfo{
												   STYPE(COMMAND_BUFFER_SUBMIT_INFO),
												   nullptr,
												   image_memory_barrier_cmd,
												   0u
											   });
		}

		VkSubmitInfo2 submit_info{}; // Best of Zach 2. Best of, best of Zach 2.
		submit_info.sType                    = STYPE(SUBMIT_INFO_2);
		submit_info.pNext                    = nullptr;
		submit_info.waitSemaphoreInfoCount   = p_wait_semaphore_infos.size();
		submit_info.pWaitSemaphoreInfos      = p_wait_semaphore_infos.data();
		submit_info.signalSemaphoreInfoCount = p_signal_semaphore_infos.size();
		submit_info.pSignalSemaphoreInfos    = p_signal_semaphore_infos.data();
		submit_info.commandBufferInfoCount   = command_buffer_submit_infos.size();
		submit_info.pCommandBufferInfos      = command_buffer_submit_infos.data();

		const VkQueue queue{p_ctx->queues[static_cast<uint32>(p_queue_type)]};

		mtx_lock(&p_ctx->queueMutexes[static_cast<uint32>(p_queue_type)]); // Lock for the submit
		vkQueueSubmit2(queue, 1u, &submit_info, nullptr);
		mtx_unlock(&p_ctx->queueMutexes[static_cast<uint32>(p_queue_type)]);
	}

	auto queue_submit(GPUContext *                               p_ctx, EQueueType p_queue_type, InitialiserList<const CommandListHandle> p_cmd_lists,
					  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos, InitialiserList<const SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void
	{
		Vector<VkSemaphoreSubmitInfo> wait_infos(p_wait_semaphore_infos.size());
		for (const auto &[i, wait_info]: std::views::enumerate(p_wait_semaphore_infos))
		{
			const VkSemaphore semaphore{p_ctx->semaphores[wait_info.semaphore].semaphore};

			auto &dst_wait_info{wait_infos[i]};
			dst_wait_info.sType     = STYPE(SEMAPHORE_SUBMIT_INFO);
			dst_wait_info.pNext     = nullptr;
			dst_wait_info.semaphore = semaphore;
			dst_wait_info.value     = wait_info.value;
		}

		Vector<VkSemaphoreSubmitInfo> signal_infos(p_signal_semaphore_infos.size());
		for (const auto &[i, signal_info]: std::views::enumerate(p_signal_semaphore_infos))
		{
			const VkSemaphore semaphore{p_ctx->semaphores[signal_info.semaphore].semaphore};

			auto &dst_signal_info{signal_infos[i]};
			dst_signal_info.sType     = STYPE(SEMAPHORE_SUBMIT_INFO);
			dst_signal_info.pNext     = nullptr;
			dst_signal_info.semaphore = semaphore;
			dst_signal_info.value     = signal_info.value;
		}

		submit(p_ctx, p_queue_type, p_cmd_lists, wait_infos, signal_infos);
	}

	auto cmd_execute_cmd_lists(GPUContext *p_ctx, CommandListHandle p_primary_command_list, InitialiserList<const CommandListHandle> p_secondary_command_lists) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_primary_command_list]};
		TST_ASSERT(!cmd.secondary && cmd.open);

		Vector<VkCommandBuffer> secondary_buffers(p_secondary_command_lists.size());
		for (auto &&[i, secondary_buffer]: std::views::enumerate(secondary_buffers))
		{
			const CommandList &secondary{p_ctx->commandLists[p_secondary_command_lists[i]]};
			TST_ASSERT(secondary.secondary && !secondary.open);
			secondary_buffer = secondary.cmd;
		}

		vkCmdExecuteCommands(cmd.cmd, secondary_buffers.size(), secondary_buffers.data());
	}

	auto cmd_copy_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_src_buffer, BufferHandle p_dst_buffer, uint64 p_size, uint64 p_src_offset,
						 uint64      p_dst_offset) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const Buffer &src_buffer{p_ctx->buffers[p_src_buffer]};
		const Buffer &dst_buffer{p_ctx->buffers[p_dst_buffer]};

		VkBufferCopy2 copy_region{};
		copy_region.sType     = STYPE(BUFFER_COPY_2);
		copy_region.pNext     = nullptr;
		copy_region.size      = p_size;
		copy_region.srcOffset = p_src_offset;
		copy_region.dstOffset = p_dst_offset;

		VkCopyBufferInfo2 copy_buffer_info{};
		copy_buffer_info.sType       = STYPE(COPY_BUFFER_INFO_2);
		copy_buffer_info.pNext       = nullptr;
		copy_buffer_info.regionCount = 1u;
		copy_buffer_info.pRegions    = &copy_region;
		copy_buffer_info.srcBuffer   = src_buffer.buffer;
		copy_buffer_info.dstBuffer   = dst_buffer.buffer;

		vkCmdCopyBuffer2(cmd.cmd, &copy_buffer_info);
	}

	auto cmd_copy_buffer_to_texture(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_src_buffer, TextureHandle p_dst_texture, uint64 p_src_offset,
									uint32      p_mip_level, uint32      p_base_layer, uint32     p_layer_count, math::UInt3  p_extent, math::Int3  p_offset) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Buffer & src_buffer{p_ctx->buffers[p_src_buffer]};
		Texture *dst_texture{p_ctx->textures.tryGet(p_dst_texture)};

		TST_ASSERT_MSG(dst_texture->desc.usage & ETextureUsageFlagBits::eTransferDst, "Texture was not created with the usage of transfer dst");
		TST_ASSERT_MSG(p_mip_level < dst_texture->desc.mipCount, "Texture mip level is out of range");
		TST_ASSERT_MSG(p_base_layer + p_layer_count <= dst_texture->desc.layerCount, "Texture layer range is out of range");

		if (p_extent.x == 0u)
			p_extent.x = (dst_texture->desc.extent.x >> p_mip_level) ? (dst_texture->desc.extent.x >> p_mip_level) : 1u;
		if (p_extent.y == 0u)
			p_extent.y = (dst_texture->desc.extent.y >> p_mip_level) ? (dst_texture->desc.extent.y >> p_mip_level) : 1u;
		if (p_extent.z == 0u)
			p_extent.z = (dst_texture->desc.extent.z >> p_mip_level) ? (dst_texture->desc.extent.z >> p_mip_level) : 1u;

		mtx_lock(&p_ctx->undefinedTexturesMutex);

		auto undefined_it{std::ranges::find(p_ctx->undefinedTextures, p_dst_texture)};
		if (undefined_it != p_ctx->undefinedTextures.end())
		{
			VkImageMemoryBarrier2 barrier{};
			barrier.sType               = STYPE(IMAGE_MEMORY_BARRIER_2);
			barrier.pNext               = nullptr;
			barrier.srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			barrier.dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.srcAccessMask       = VK_ACCESS_2_NONE;
			barrier.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image               = dst_texture->image;

			VkImageSubresourceRange &subresource_range{barrier.subresourceRange};
			subresource_range.aspectMask     = get_vk_image_aspect_mask(dst_texture->desc.format);
			subresource_range.baseMipLevel   = 0u;
			subresource_range.levelCount     = dst_texture->desc.mipCount;
			subresource_range.baseArrayLayer = 0u;
			subresource_range.layerCount     = dst_texture->desc.layerCount; // TODO: More options

			VkDependencyInfo dependency_info{};
			dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
			dependency_info.pNext                    = nullptr;
			dependency_info.imageMemoryBarrierCount  = 1u;
			dependency_info.pImageMemoryBarriers     = &barrier;
			dependency_info.memoryBarrierCount       = 0u;
			dependency_info.bufferMemoryBarrierCount = 0u;

			vkCmdPipelineBarrier2(cmd.cmd, &dependency_info);

			undefined_it = p_ctx->undefinedTextures.erase(undefined_it);
		}

		mtx_unlock(&p_ctx->undefinedTexturesMutex);

		VkBufferImageCopy2 copy_region{};
		copy_region.sType             = STYPE(BUFFER_IMAGE_COPY_2);
		copy_region.pNext             = nullptr;
		copy_region.bufferOffset      = p_src_offset;
		copy_region.bufferRowLength   = 0u; // TODO: More options for advanced copy operations
		copy_region.bufferImageHeight = 0u;
		copy_region.imageSubresource  = VkImageSubresourceLayers{get_vk_image_aspect_mask(dst_texture->desc.format), p_mip_level, p_base_layer, p_layer_count};
		copy_region.imageOffset       = VkOffset3D{p_offset.x, p_offset.y, p_offset.z};
		copy_region.imageExtent       = VkExtent3D{p_extent.x, p_extent.y, p_extent.z};

		VkCopyBufferToImageInfo2 copy_buffer_to_image_info{};
		copy_buffer_to_image_info.sType          = STYPE(COPY_BUFFER_TO_IMAGE_INFO_2);
		copy_buffer_to_image_info.pNext          = nullptr;
		copy_buffer_to_image_info.regionCount    = 1u;
		copy_buffer_to_image_info.pRegions       = &copy_region;
		copy_buffer_to_image_info.srcBuffer      = src_buffer.buffer;
		copy_buffer_to_image_info.dstImage       = dst_texture->image;
		copy_buffer_to_image_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL; // Thank you VK_KHR_unified_image_layouts :)

		vkCmdCopyBufferToImage2(cmd.cmd, &copy_buffer_to_image_info);
	}

	auto cmd_fill_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_dst_offset, uint64 p_size, uint32 p_data) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Buffer &buffer{p_ctx->buffers[p_buffer]};

		vkCmdFillBuffer(cmd.cmd, buffer.buffer, p_dst_offset, p_size, p_data);
	}

	auto cmd_pipeline_barrier(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const BufferMemoryBarrier> p_buffer_memory_barriers,
							  InitialiserList<const TextureMemoryBarrier> p_texture_memory_barriers) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Vector<VkBufferMemoryBarrier2> buffer_barriers(p_buffer_memory_barriers.size());
		for (auto &&[i, barrier]: std::views::enumerate(buffer_barriers))
		{
			const auto &src_barrier{p_buffer_memory_barriers[i]};

			barrier.sType = STYPE(BUFFER_MEMORY_BARRIER_2);
			barrier.pNext = nullptr;

			barrier.buffer = p_ctx->buffers[src_barrier.buffer].buffer;
			barrier.offset = src_barrier.offset;
			barrier.size   = src_barrier.size;

			barrier.srcStageMask = get_vk_pipeline_stage_flags(src_barrier.srcStageMask);
			barrier.dstStageMask = get_vk_pipeline_stage_flags(src_barrier.dstStageMask);

			barrier.srcAccessMask = get_vk_access_flags(src_barrier.srcAccessMask);
			barrier.dstAccessMask = get_vk_access_flags(src_barrier.dstAccessMask);

			barrier.srcQueueFamilyIndex = p_ctx->queueFamilyIndices.get(src_barrier.srcQueue);
			barrier.dstQueueFamilyIndex = p_ctx->queueFamilyIndices.get(src_barrier.dstQueue);
		}

		Vector<VkImageMemoryBarrier2> image_barriers(p_texture_memory_barriers.size());
		for (auto &&[i, barrier]: std::views::enumerate(image_barriers))
		{
			const auto &   src_barrier{p_texture_memory_barriers[i]};
			const Texture &texture{p_ctx->textures[src_barrier.texture]};

			barrier.sType = STYPE(IMAGE_MEMORY_BARRIER_2);
			barrier.pNext = nullptr;

			barrier.image = texture.image;

			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

			barrier.srcStageMask = get_vk_pipeline_stage_flags(src_barrier.srcStageMask);
			barrier.dstStageMask = get_vk_pipeline_stage_flags(src_barrier.dstStageMask);

			barrier.srcAccessMask = get_vk_access_flags(src_barrier.srcAccessMask);
			barrier.dstAccessMask = get_vk_access_flags(src_barrier.dstAccessMask);

			barrier.subresourceRange.aspectMask     = get_vk_image_aspect_mask(texture.desc.format);
			barrier.subresourceRange.baseArrayLayer = src_barrier.baseArrayLayer;
			barrier.subresourceRange.baseMipLevel   = src_barrier.baseMip;
			barrier.subresourceRange.layerCount     = (src_barrier.layerCount == UINT32_MAX) ? texture.desc.layerCount : src_barrier.layerCount;
			barrier.subresourceRange.levelCount     = (src_barrier.mipCount == UINT32_MAX) ? texture.desc.mipCount : src_barrier.mipCount;

			barrier.srcQueueFamilyIndex = p_ctx->queueFamilyIndices.get(src_barrier.srcQueue);
			barrier.dstQueueFamilyIndex = p_ctx->queueFamilyIndices.get(src_barrier.dstQueue);
		}

		VkDependencyInfo dependency_info{};
		dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
		dependency_info.pNext                    = nullptr;
		dependency_info.imageMemoryBarrierCount  = image_barriers.size();
		dependency_info.pImageMemoryBarriers     = image_barriers.data();
		dependency_info.bufferMemoryBarrierCount = buffer_barriers.size();
		dependency_info.pBufferMemoryBarriers    = buffer_barriers.data();
		dependency_info.memoryBarrierCount       = 0u;

		vkCmdPipelineBarrier2(cmd.cmd, &dependency_info);
	}

	auto cmd_generate_mipmaps(GPUContext *p_ctx, CommandListHandle p_cmd_list, TextureHandle p_texture) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Texture *dst_texture{p_ctx->textures.tryGet(p_texture)};
		TST_PERMA_ASSERT(dst_texture);
		TST_ASSERT_MSG(dst_texture->desc.mipCount > 1u, "Texture does not contain mip levels to generate");
		TST_ASSERT_MSG(dst_texture->desc.usage & ETextureUsageFlagBits::eTransferSrc, "Texture was not created with the usage of transfer src");
		TST_ASSERT_MSG(dst_texture->desc.usage & ETextureUsageFlagBits::eTransferDst, "Texture was not created with the usage of transfer dst");

		int32 mip_width{static_cast<int32>(dst_texture->desc.extent.x)};
		int32 mip_height{static_cast<int32>(dst_texture->desc.extent.y)};

		const VkImageAspectFlags image_aspect{get_vk_image_aspect_mask(dst_texture->desc.format)};

		VkImageMemoryBarrier2 memory_barrier{};
		memory_barrier.sType                           = STYPE(IMAGE_MEMORY_BARRIER_2);
		memory_barrier.pNext                           = nullptr;
		memory_barrier.image                           = dst_texture->image;
		memory_barrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
		memory_barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
		memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		memory_barrier.subresourceRange.aspectMask     = image_aspect;
		memory_barrier.subresourceRange.baseArrayLayer = 0u;
		memory_barrier.subresourceRange.layerCount     = 1u;
		memory_barrier.subresourceRange.levelCount     = 1u;

		for (uint32 i{1u}; i < dst_texture->desc.mipCount; ++i)
		{
			Array<VkImageMemoryBarrier2, 2u> pre_blit_barriers;

			// Src mip
			pre_blit_barriers[0u].sType                         = STYPE(IMAGE_MEMORY_BARRIER_2);
			pre_blit_barriers[0u].pNext                         = nullptr;
			pre_blit_barriers[0u]                               = memory_barrier;
			pre_blit_barriers[0u].subresourceRange.baseMipLevel = i - 1u;
			pre_blit_barriers[0u].srcStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			pre_blit_barriers[0u].srcAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			pre_blit_barriers[0u].dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			pre_blit_barriers[0u].dstAccessMask                 = VK_ACCESS_2_TRANSFER_READ_BIT;

			// Dst mip
			pre_blit_barriers[1u].sType                         = STYPE(IMAGE_MEMORY_BARRIER_2);
			pre_blit_barriers[1u].pNext                         = nullptr;
			pre_blit_barriers[1u]                               = memory_barrier;
			pre_blit_barriers[1u].subresourceRange.baseMipLevel = i;
			pre_blit_barriers[1u].srcStageMask                  = VK_PIPELINE_STAGE_2_NONE;
			pre_blit_barriers[1u].srcAccessMask                 = VK_ACCESS_2_NONE;
			pre_blit_barriers[1u].dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			pre_blit_barriers[1u].dstAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;

			VkDependencyInfo dependency_info{};
			dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
			dependency_info.pNext                    = nullptr;
			dependency_info.imageMemoryBarrierCount  = 2u;
			dependency_info.pImageMemoryBarriers     = pre_blit_barriers.data();
			dependency_info.memoryBarrierCount       = 0u;
			dependency_info.bufferMemoryBarrierCount = 0u;

			vkCmdPipelineBarrier2(cmd.cmd, &dependency_info);

			VkImageBlit2 image_blit{};
			image_blit.sType = STYPE(IMAGE_BLIT_2);
			image_blit.pNext = nullptr;
			auto &src_offsets{image_blit.srcOffsets};
			auto &dst_offsets{image_blit.dstOffsets};

			src_offsets[0] = VkOffset3D{0, 0, 0};
			src_offsets[1] = VkOffset3D{mip_width, mip_height, 1};

			int32 next_width  = mip_width > 1 ? mip_width / 2 : 1;
			int32 next_height = mip_height > 1 ? mip_height / 2 : 1;

			dst_offsets[0] = VkOffset3D{0, 0, 0};
			dst_offsets[1] = VkOffset3D{next_width, next_height, 1};

			image_blit.srcSubresource = VkImageSubresourceLayers{image_aspect, i - 1, 0, 1};
			image_blit.dstSubresource = VkImageSubresourceLayers{image_aspect, i, 0, 1};

			VkBlitImageInfo2 blit_info{};
			blit_info.sType          = STYPE(BLIT_IMAGE_INFO_2);
			blit_info.pNext          = nullptr;
			blit_info.srcImage       = dst_texture->image;
			blit_info.dstImage       = dst_texture->image;
			blit_info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
			blit_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
			blit_info.regionCount    = 1u;
			blit_info.pRegions       = &image_blit;
			blit_info.filter         = VK_FILTER_LINEAR;

			vkCmdBlitImage2(cmd.cmd, &blit_info);

			if (mip_width > 1)
				mip_width /= 2;
			if (mip_height > 1)
				mip_height /= 2;

			VkImageMemoryBarrier2 post_blit_barrier         = memory_barrier;
			post_blit_barrier.sType                         = STYPE(IMAGE_MEMORY_BARRIER_2);
			post_blit_barrier.pNext                         = nullptr;
			post_blit_barrier.subresourceRange.baseMipLevel = i - 1u;
			post_blit_barrier.srcStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			post_blit_barrier.srcAccessMask                 = VK_ACCESS_2_TRANSFER_READ_BIT;
			post_blit_barrier.dstStageMask                  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			post_blit_barrier.dstAccessMask                 = VK_ACCESS_2_SHADER_READ_BIT;

			VkDependencyInfo post_dependency_info{};
			post_dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
			post_dependency_info.pNext                    = nullptr;
			post_dependency_info.imageMemoryBarrierCount  = 1u;
			post_dependency_info.pImageMemoryBarriers     = &post_blit_barrier;
			post_dependency_info.memoryBarrierCount       = 0u;
			post_dependency_info.bufferMemoryBarrierCount = 0u;

			vkCmdPipelineBarrier2(cmd.cmd, &post_dependency_info);
		}

		VkImageMemoryBarrier2 final_barrier         = memory_barrier;
		final_barrier.subresourceRange.baseMipLevel = dst_texture->desc.mipCount - 1u;
		final_barrier.srcStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		final_barrier.srcAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		final_barrier.dstStageMask                  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		final_barrier.dstAccessMask                 = VK_ACCESS_2_SHADER_READ_BIT;

		VkDependencyInfo final_dependency_info{};
		final_dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
		final_dependency_info.pNext                    = nullptr;
		final_dependency_info.imageMemoryBarrierCount  = 1u;
		final_dependency_info.pImageMemoryBarriers     = &final_barrier;
		final_dependency_info.memoryBarrierCount       = 0u;
		final_dependency_info.bufferMemoryBarrierCount = 0u;

		vkCmdPipelineBarrier2(cmd.cmd, &final_dependency_info);
	}

	auto cmd_begin_rendering(GPUContext *p_ctx, CommandListHandle p_cmd_list, const RenderingInfo &p_rendering_info) -> void
	{
		Vector<VkRenderingAttachmentInfo> colour_attachments(p_rendering_info.colourAttachments.size());
		for (auto &&[i, attachment]: std::views::enumerate(colour_attachments))
		{
			auto &         src_attachment{p_rendering_info.colourAttachments[i]};
			const Texture *render_target{p_ctx->textures.tryGet(src_attachment.renderTarget)};
			TST_ASSERT(render_target);

			attachment.sType = STYPE(RENDERING_ATTACHMENT_INFO);
			attachment.pNext = nullptr;

			attachment.imageView   = render_target->imageView;
			attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			attachment.loadOp     = get_vk_load_op(src_attachment.usageOp);
			attachment.storeOp    = get_vk_store_op(src_attachment.usageOp);
			attachment.clearValue = *reinterpret_cast<const VkClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (p_ctx->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture *resolve_render_target{p_ctx->textures.tryGet(src_attachment.resolveTarget)};
				TST_ASSERT(resolve_render_target);
				attachment.resolveImageView   = resolve_render_target->imageView;
				attachment.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
				attachment.resolveMode        = get_vk_resolve_mode(src_attachment.resolveMode);
			}
		}

		VkRenderingAttachmentInfo depth_attachment{};
		if (p_rendering_info.depthAttachment.has_value())
		{
			auto &         src_attachment{p_rendering_info.depthAttachment.value()};
			const Texture *render_target{p_ctx->textures.tryGet(src_attachment.renderTarget)};
			TST_ASSERT(render_target);

			depth_attachment.sType = STYPE(RENDERING_ATTACHMENT_INFO);
			depth_attachment.pNext = nullptr;

			depth_attachment.imageView   = render_target->imageView;
			depth_attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			depth_attachment.loadOp     = get_vk_load_op(src_attachment.usageOp);
			depth_attachment.storeOp    = get_vk_store_op(src_attachment.usageOp);
			depth_attachment.clearValue = *reinterpret_cast<const VkClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (p_ctx->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture *resolve_render_target{p_ctx->textures.tryGet(src_attachment.resolveTarget)};
				TST_ASSERT(resolve_render_target);
				depth_attachment.resolveImageView   = resolve_render_target->imageView;
				depth_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
				depth_attachment.resolveMode        = get_vk_resolve_mode(src_attachment.resolveMode);
			}
		}

		VkRenderingAttachmentInfo stencil_attachment{};
		if (p_rendering_info.stencilAttachment.has_value())
		{
			auto &         src_attachment{p_rendering_info.stencilAttachment.value()};
			const Texture *render_target{p_ctx->textures.tryGet(src_attachment.renderTarget)};
			TST_ASSERT(render_target);

			depth_attachment.sType = STYPE(RENDERING_ATTACHMENT_INFO);
			depth_attachment.pNext = nullptr;

			stencil_attachment.imageView   = render_target->imageView;
			stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			stencil_attachment.loadOp     = get_vk_load_op(src_attachment.usageOp);
			stencil_attachment.storeOp    = get_vk_store_op(src_attachment.usageOp);
			stencil_attachment.clearValue = *reinterpret_cast<const VkClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (p_ctx->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture *resolve_render_target{p_ctx->textures.tryGet(src_attachment.resolveTarget)};
				TST_ASSERT(resolve_render_target);
				stencil_attachment.resolveImageView   = resolve_render_target->imageView;
				stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
				stencil_attachment.resolveMode        = get_vk_resolve_mode(src_attachment.resolveMode);
			}
		}

		VkRenderingInfo rendering_info{};
		rendering_info.sType      = STYPE(RENDERING_INFO);
		rendering_info.pNext      = nullptr;
		rendering_info.renderArea = VkRect2D{
			VkOffset2D{p_rendering_info.renderArea.offset.x, p_rendering_info.renderArea.offset.y},
			VkExtent2D{p_rendering_info.renderArea.size.x, p_rendering_info.renderArea.size.y}
		};
		rendering_info.layerCount           = 1u;
		rendering_info.viewMask             = 0u; // I don't even know what ts is
		rendering_info.colorAttachmentCount = colour_attachments.size();
		rendering_info.pColorAttachments    = colour_attachments.data();
		rendering_info.pDepthAttachment     = p_rendering_info.depthAttachment.has_value() ? &depth_attachment : nullptr;
		rendering_info.pStencilAttachment   = p_rendering_info.stencilAttachment.has_value() ? &stencil_attachment : nullptr;

		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		rendering_info.flags = p_rendering_info.useSecondaryCommandBuffers ? VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT : 0u;
		vkCmdBeginRendering(cmd.cmd, &rendering_info);
	}

	auto cmd_end_rendering(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		vkCmdEndRendering(cmd.cmd);
	}

	auto cmd_push_data(GPUContext *p_ctx, CommandListHandle p_cmd_list, const void *p_data, uint64 p_size, uint32 p_offset) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		VkPushDataInfoEXT push_data_info{};
		push_data_info.sType        = STYPE(PUSH_DATA_INFO_EXT);
		push_data_info.pNext        = nullptr;
		push_data_info.data.address = p_data;
		push_data_info.data.size    = p_size;
		push_data_info.offset       = p_offset;

		vkCmdPushDataEXT(cmd.cmd, &push_data_info);
	}

	auto cmd_bind_shaders(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const ShaderHandle> p_shaders) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		static constexpr uint32                                             c_shader_stage_count{6u};
		static constexpr Array<VkShaderStageFlagBits, c_shader_stage_count> c_shader_stages{
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
			VK_SHADER_STAGE_MESH_BIT_EXT,
			VK_SHADER_STAGE_TASK_BIT_EXT,
		};

		Array<VkShaderEXT, c_shader_stage_count> shaders;
		shaders.fill(nullptr);

		for (const ShaderHandle handle: p_shaders)
		{
			const Shader &shader{p_ctx->shaders[handle]};

			for (uint32 i{0u}; i < c_shader_stage_count; ++i)
			{
				if (shader.stage == c_shader_stages[i])
				{
					shaders[i] = shader.shader;
					break;
				}
			}
		}

		vkCmdBindShadersEXT(cmd.cmd, c_shader_stage_count, c_shader_stages.data(), shaders.data());
	}

	auto cmd_bind_resource_heap(GPUContext *p_ctx, CommandListHandle p_cmd_list, ResourceDescriptorHeapHandle p_resource_heap) -> void
	{
		CommandList &                 cmd{p_ctx->commandLists[p_cmd_list]};
		const ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		vkCmdBindResourceHeapEXT(cmd.cmd, &resource_heap.bindInfo);
	}

	auto cmd_bind_sampler_heap(GPUContext *p_ctx, CommandListHandle p_cmd_list, SamplerDescriptorHeapHandle p_sampler_heap) -> void
	{
		CommandList &                cmd{p_ctx->commandLists[p_cmd_list]};
		const SamplerDescriptorHeap &sampler_heap{p_ctx->samplerHeaps[p_sampler_heap]};

		vkCmdBindSamplerHeapEXT(cmd.cmd, &sampler_heap.bindInfo);
	}

	auto cmd_bind_index_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_index_buffer, EIndexType p_index_type) -> void
	{
		CommandList & cmd{p_ctx->commandLists[p_cmd_list]};
		const Buffer *index_buffer{p_ctx->buffers.tryGet(p_index_buffer)};
		TST_ASSERT_MSG(index_buffer, "Index buffer cannot be null");

		// TODO: Maybe I should actually support the traditional vertex input pipeline as well...
		vkCmdSetVertexInputEXT(cmd.cmd, 0u, nullptr, 0u, nullptr);

		const VkIndexType index_type{get_vk_index_type(p_index_type)};

		vkCmdBindIndexBuffer(cmd.cmd, index_buffer->buffer, 0u, index_type);
	}

	auto cmd_set_primitive_topology(GPUContext *p_ctx, CommandListHandle p_cmd_list, EPrimitiveTopology p_primitive_topology) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkPrimitiveTopology primitive_topology{get_vk_primitive_topology(p_primitive_topology)};
		vkCmdSetPrimitiveTopology(cmd.cmd, primitive_topology);
	}

	auto cmd_set_primitive_restart(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable, uint32 p_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetPrimitiveRestartEnable(cmd.cmd, p_enable);
		if (p_enable)
			vkCmdSetPrimitiveRestartIndexEXT(cmd.cmd, p_index); // I actually have no idea what this does...
	}

	auto cmd_set_viewport(GPUContext *p_ctx, CommandListHandle p_cmd_list, const math::Viewport &p_viewport) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkViewport viewport{*reinterpret_cast<const VkViewport *>(&p_viewport)}; // math::Viewport has the same memory layout as VkViewport
		vkCmdSetViewportWithCount(cmd.cmd, 1u, &viewport);
	}

	auto cmd_set_scissor(GPUContext *p_ctx, CommandListHandle p_cmd_list, const math::Rect &p_scissor_rect) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkRect2D scissor{VkOffset2D{p_scissor_rect.offset.x, p_scissor_rect.offset.y}, VkExtent2D{p_scissor_rect.size.x, p_scissor_rect.size.y}};
		vkCmdSetScissorWithCount(cmd.cmd, 1u, &scissor);
	}

	auto cmd_set_rasterizer_discard_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		vkCmdSetRasterizerDiscardEnable(cmd.cmd, p_enable);
	}

	auto cmd_set_polygon_mode(GPUContext *p_ctx, CommandListHandle p_cmd_list, EPolygonMode p_polygon_mode) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkPolygonMode polygon_mode{get_vk_polygon_mode(p_polygon_mode)};
		vkCmdSetPolygonModeEXT(cmd.cmd, polygon_mode);
	}

	auto cmd_set_cull_mode(GPUContext *p_ctx, CommandListHandle p_cmd_list, ECullMode p_cull_mode) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkCullModeFlagBits cull_mode{get_vk_cull_mode(p_cull_mode)};
		vkCmdSetCullMode(cmd.cmd, cull_mode);
	}

	auto cmd_set_front_face(GPUContext *p_ctx, CommandListHandle p_cmd_list, EFrontFace p_front_face) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkFrontFace front_face{(p_front_face == EFrontFace::eCW) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE};
		vkCmdSetFrontFace(cmd.cmd, front_face);
	}

	auto cmd_set_depth_bias(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable, float32 p_constant_factor, float32 p_clamp, float32 p_slope_factor) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetDepthBiasEnable(cmd.cmd, p_enable);
		if (p_enable)
			vkCmdSetDepthBias(cmd.cmd, p_constant_factor, p_clamp, p_slope_factor);
	}

	auto cmd_set_line_width(GPUContext *p_ctx, CommandListHandle p_cmd_list, float32 p_line_width) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetLineWidth(cmd.cmd, p_line_width);
	}

	auto cmd_set_rasterization_samples(GPUContext *p_ctx, CommandListHandle p_cmd_list, ESampleCount p_sample_count) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const VkSampleCountFlagBits sample_count{get_vk_sample_count(p_sample_count)};
		vkCmdSetRasterizationSamplesEXT(cmd.cmd, sample_count);

		constexpr VkSampleMask sample_mask{0xFFFFFFFF};
		vkCmdSetSampleMaskEXT(cmd.cmd, sample_count, &sample_mask); // I don't think I should expose this

		vkCmdSetAlphaToCoverageEnableEXT(cmd.cmd, false); // I don't think I should expose this either
	}

	auto cmd_set_depth_state(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_test_enable, bool p_write_enable, bool p_clamp_enable,
							 ECompareOp  p_compare_op) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetDepthTestEnable(cmd.cmd, p_test_enable);
		if (p_test_enable)
		{
			vkCmdSetDepthWriteEnable(cmd.cmd, p_write_enable);
			vkCmdSetDepthClampEnableEXT(cmd.cmd, p_clamp_enable);

			const VkCompareOp compare_op{get_vk_compare_op(p_compare_op)};
			vkCmdSetDepthCompareOp(cmd.cmd, compare_op);
		}
	}

	auto cmd_set_stencil_state(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_test_enable) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetStencilTestEnable(cmd.cmd, p_test_enable);
	}

	auto cmd_set_colour_write_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const bool32> p_enables) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetColorWriteEnableEXT(cmd.cmd, p_enables.size(), p_enables.data());
	}

	auto cmd_set_colour_write_mask(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const EColourComponentFlags> p_masks,
								   uint32      p_first_attachment_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		Vector<VkColorComponentFlags> component_flags(p_masks.size());
		for (uint32 i{0u}; i < p_masks.size(); ++i)
			component_flags[i] = get_vk_colour_component_flags(p_masks[i]);

		// VkColorComponentFlags flags{VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
		// vkCmdSetColorWriteMaskEXT(cmd.cmd, p_first_attachment_index, 1u,&flags );

		vkCmdSetColorWriteMaskEXT(cmd.cmd, p_first_attachment_index, component_flags.size(), component_flags.data());
	}

	auto cmd_set_colour_blend_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const bool32> p_enables, uint32 p_first_attachment_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdSetColorBlendEnableEXT(cmd.cmd, p_first_attachment_index, p_enables.size(), p_enables.data());
	}

	auto cmd_set_colour_blend_equation(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const ColourBlendEquation> p_blend_equations,
									   uint32      p_first_attachment_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		Vector<VkColorBlendEquationEXT> colour_blend_equations(p_blend_equations.size());
		for (auto &&[i, equation]: std::views::enumerate(colour_blend_equations))
		{
			const auto &src_equation{p_blend_equations[i]};
			equation.srcColorBlendFactor = get_vk_colour_blend_factor(src_equation.srcColorBlendFactor);
			equation.dstColorBlendFactor = get_vk_colour_blend_factor(src_equation.dstColorBlendFactor);
			equation.colorBlendOp        = get_vk_colour_blend_op(src_equation.colorBlendOp);
			equation.srcAlphaBlendFactor = get_vk_colour_blend_factor(src_equation.srcAlphaBlendFactor);
			equation.dstAlphaBlendFactor = get_vk_colour_blend_factor(src_equation.dstAlphaBlendFactor);
			equation.alphaBlendOp        = get_vk_colour_blend_op(src_equation.alphaBlendOp);
		}
		vkCmdSetColorBlendEquationEXT(cmd.cmd, p_first_attachment_index, colour_blend_equations.size(), colour_blend_equations.data());
	}

	auto cmd_draw(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_vertex_count, uint32 p_instance_count, uint32 p_first_vertex, uint32 p_first_instance) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdDraw(cmd.cmd, p_vertex_count, p_instance_count, p_first_vertex, p_first_instance);
	}

	auto cmd_draw_indexed(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_index_count, uint32 p_instance_count, uint32 p_first_index, int32 p_vertex_offset,
						  uint32      p_first_instance) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdDrawIndexed(cmd.cmd, p_index_count, p_instance_count, p_first_index, p_vertex_offset, p_first_instance);
	}

	auto cmd_draw_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count, uint32 p_stride) -> void
	{
		CommandList & cmd{p_ctx->commandLists[p_cmd_list]};
		const Buffer &buffer{p_ctx->buffers[p_buffer]};

		vkCmdDrawIndirect(cmd.cmd, buffer.buffer, p_offset, p_draw_count, p_stride);
	}

	auto cmd_draw_indexed_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count, uint32 p_stride) -> void
	{
		CommandList & cmd{p_ctx->commandLists[p_cmd_list]};
		const Buffer &buffer{p_ctx->buffers[p_buffer]};

		vkCmdDrawIndexedIndirect(cmd.cmd, buffer.buffer, p_offset, p_draw_count, p_stride);
	}

	auto cmd_draw_mesh_tasks(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_group_count_x, uint32 p_group_count_y, uint32 p_group_count_z) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdDrawMeshTasksEXT(cmd.cmd, p_group_count_x, p_group_count_y, p_group_count_z);
	}

	auto cmd_draw_mesh_tasks_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
									  uint32      p_stride) -> void
	{
		CommandList & cmd{p_ctx->commandLists[p_cmd_list]};
		const Buffer &buffer{p_ctx->buffers[p_buffer]};

		vkCmdDrawMeshTasksIndirectEXT(cmd.cmd, buffer.buffer, p_offset, p_draw_count, p_stride);
	}

	auto cmd_draw_mesh_tasks_indirect_count(GPUContext * p_ctx, CommandListHandle p_cmd_list, BufferHandle      p_indirect_buffer, uint64 p_indirect_buffer_offset,
											BufferHandle p_count_buffer, uint64   p_count_buffer_offset, uint32 p_max_draw_count, uint32  p_stride) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const Buffer &indirect_buffer{p_ctx->buffers[p_indirect_buffer]};
		const Buffer &count_buffer{p_ctx->buffers[p_count_buffer]};

		vkCmdDrawMeshTasksIndirectCountEXT(cmd.cmd, indirect_buffer.buffer, p_indirect_buffer_offset, count_buffer.buffer, p_count_buffer_offset, p_max_draw_count,
										   p_stride);
	}

	auto cmd_dispatch(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_work_groups_x, uint32 p_work_groups_y, uint32 p_work_groups_z) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vkCmdDispatch(cmd.cmd, p_work_groups_x, p_work_groups_y, p_work_groups_z);
	}

	static auto get_texture_base_heap_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};
		return p_heap_slot + (resource_heap.imageSegmentOffset / resource_heap.imageDescriptorSize);
	}

	static auto get_segment_relative_texture_heap_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};
		return p_heap_slot - (resource_heap.imageSegmentOffset / resource_heap.imageDescriptorSize);
	}

	auto resource_descriptor_heap_create(GPUContext *p_ctx, const ResourceDescriptorHeapDesc &p_desc) -> ResourceDescriptorHeapHandle
	{
		const auto &heap_props{p_ctx->descriptorHeapProperties};

		ResourceDescriptorHeap resource_heap{};

		mtx_init(&resource_heap.bufferSlotMutex, mtx_plain);
		mtx_init(&resource_heap.imageSlotMutex, mtx_plain);

		resource_heap.bufferSlotAllocator = {p_desc.maxBufferDescriptors};
		resource_heap.imageSlotAllocator  = {p_desc.maxImageDescriptors};

		resource_heap.bufferDescriptorSize = TST_ALIGN(heap_props.bufferDescriptorSize, heap_props.bufferDescriptorAlignment);
		resource_heap.imageDescriptorSize  = TST_ALIGN(heap_props.imageDescriptorSize, heap_props.imageDescriptorAlignment);

		resource_heap.bufferSegmentSize = (p_desc.maxBufferDescriptors * resource_heap.bufferDescriptorSize);

		resource_heap.imageSegmentOffset = TST_ALIGN(resource_heap.bufferSegmentSize, heap_props.imageDescriptorAlignment);
		resource_heap.imageSegmentSize   = (p_desc.maxImageDescriptors * resource_heap.imageDescriptorSize);

		VkDeviceSize resource_heap_size{
			TST_ALIGN(resource_heap.bufferSegmentSize + resource_heap.imageSegmentSize + heap_props.minResourceHeapReservedRange, heap_props.resourceHeapAlignment)
		};

		VkBufferCreateInfo resource_heap_create_info{};
		resource_heap_create_info.sType       = STYPE(BUFFER_CREATE_INFO);
		resource_heap_create_info.pNext       = nullptr;
		resource_heap_create_info.usage       = VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
		resource_heap_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		resource_heap_create_info.size        = resource_heap_size;

		VmaAllocationCreateInfo heap_allocation_create_info{};
		heap_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		heap_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocation_info;
		vmaCreateBuffer(p_ctx->allocator, &resource_heap_create_info, &heap_allocation_create_info, &resource_heap.heapBuffer, &resource_heap.heapAllocation,
						&allocation_info);

		resource_heap.mappedHeapMemory = allocation_info.pMappedData;

		VkBufferDeviceAddressInfo device_address_info{};
		device_address_info.sType  = STYPE(BUFFER_DEVICE_ADDRESS_INFO);
		device_address_info.pNext  = nullptr;
		device_address_info.buffer = resource_heap.heapBuffer;
		const VkDeviceAddress device_address{vkGetBufferDeviceAddress(p_ctx->logicalDevice, &device_address_info)};

		VkDeviceAddressRangeKHR device_address_range{};
		device_address_range.address = device_address;
		device_address_range.size    = resource_heap_size;

		VkBindHeapInfoEXT &bind_info{resource_heap.bindInfo};
		bind_info.sType               = STYPE(BIND_HEAP_INFO_EXT);
		bind_info.pNext               = nullptr;
		bind_info.heapRange           = device_address_range;
		bind_info.reservedRangeOffset = resource_heap_size - heap_props.minResourceHeapReservedRange;
		bind_info.reservedRangeSize   = heap_props.minResourceHeapReservedRange;

		return p_ctx->resourceHeaps.emplace(std::move(resource_heap));
	}

	auto sampler_descriptor_heap_create(GPUContext *p_ctx, const SamplerDescriptorHeapDesc &p_desc) -> SamplerDescriptorHeapHandle
	{
		const auto &heap_props{p_ctx->descriptorHeapProperties};

		SamplerDescriptorHeap sampler_heap{};

		mtx_init(&sampler_heap.samplerSlotMutex, mtx_plain);

		sampler_heap.samplerSlotAllocator = {p_desc.maxSamplerDescriptors};

		sampler_heap.samplerDescriptorSize = TST_ALIGN(heap_props.samplerDescriptorSize, heap_props.samplerDescriptorAlignment);

		VkDeviceSize sampler_heap_size{
			TST_ALIGN(p_desc.maxSamplerDescriptors * sampler_heap.samplerDescriptorSize + heap_props.minResourceHeapReservedRange, heap_props.resourceHeapAlignment)
		};

		VkBufferCreateInfo sampler_heap_create_info{};
		sampler_heap_create_info.sType       = STYPE(BUFFER_CREATE_INFO);
		sampler_heap_create_info.pNext       = nullptr;
		sampler_heap_create_info.usage       = VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
		sampler_heap_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		sampler_heap_create_info.size        = sampler_heap_size;

		VmaAllocationCreateInfo heap_allocation_create_info{};
		heap_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		heap_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocation_info;
		vmaCreateBuffer(p_ctx->allocator, &sampler_heap_create_info, &heap_allocation_create_info, &sampler_heap.heapBuffer, &sampler_heap.heapAllocation,
						&allocation_info);

		sampler_heap.mappedHeapMemory = allocation_info.pMappedData;

		VkBufferDeviceAddressInfo device_address_info{};
		device_address_info.sType  = STYPE(BUFFER_DEVICE_ADDRESS_INFO);
		device_address_info.pNext  = nullptr;
		device_address_info.buffer = sampler_heap.heapBuffer;
		const VkDeviceAddress device_address{vkGetBufferDeviceAddress(p_ctx->logicalDevice, &device_address_info)};

		VkDeviceAddressRangeKHR device_address_range{};
		device_address_range.address = device_address;
		device_address_range.size    = sampler_heap_size;

		VkBindHeapInfoEXT &bind_info{sampler_heap.bindInfo};
		bind_info.sType               = STYPE(BIND_HEAP_INFO_EXT);
		bind_info.pNext               = nullptr;
		bind_info.heapRange           = device_address_range;
		bind_info.reservedRangeOffset = sampler_heap_size - heap_props.minSamplerHeapReservedRange;
		bind_info.reservedRangeSize   = heap_props.minSamplerHeapReservedRange;

		return p_ctx->samplerHeaps.emplace(std::move(sampler_heap));
	}

	auto resource_descriptor_heap_destroy(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap) -> void
	{
		p_ctx->resourceHeaps.destroy(p_resource_heap);
	}

	auto sampler_descriptor_heap_destroy(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap) -> void
	{
		p_ctx->samplerHeaps.destroy(p_sampler_heap);
	}

	auto resource_descriptor_heap_alloc_buffer_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		mtx_lock(&resource_heap.bufferSlotMutex);
		const uint32 slot{resource_heap.bufferSlotAllocator.allocSlot()};
		mtx_unlock(&resource_heap.bufferSlotMutex);

		return slot;
	}

	auto resource_descriptor_heap_alloc_texture_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		mtx_lock(&resource_heap.imageSlotMutex);
		const uint32 slot{resource_heap.imageSlotAllocator.allocSlot()};
		mtx_unlock(&resource_heap.imageSlotMutex);

		return get_texture_base_heap_slot(p_ctx, p_resource_heap, slot);
	}

	auto sampler_descriptor_heap_alloc_sampler_slot(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap) -> uint32
	{
		SamplerDescriptorHeap &sampler_heap{p_ctx->samplerHeaps[p_sampler_heap]};

		mtx_lock(&sampler_heap.samplerSlotMutex);
		const uint32 slot{sampler_heap.samplerSlotAllocator.allocSlot()};
		mtx_unlock(&sampler_heap.samplerSlotMutex);

		return slot;
	}

	auto resource_descriptor_heap_free_buffer_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		mtx_lock(&resource_heap.bufferSlotMutex);
		resource_heap.bufferSlotAllocator.freeSlot(p_heap_slot);
		mtx_unlock(&resource_heap.bufferSlotMutex);
	}

	auto resource_descriptor_heap_free_texture_slot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void
	{
		ResourceDescriptorHeap *resource_heap{p_ctx->resourceHeaps.tryGet(p_resource_heap)};
		TST_ASSERT_MSG(resource_heap, "Resource heap is not valid");

		TST_ASSERT(p_heap_slot != UINT32_MAX);
		const uint32 real_heap_slot{get_segment_relative_texture_heap_slot(p_ctx, p_resource_heap, p_heap_slot)};
		TST_ASSERT(real_heap_slot != UINT32_MAX);

		if (real_heap_slot > resource_heap->imageSlotAllocator.capacity())
		{
			std::printf("Heap slot is: %u", real_heap_slot);
			TST_PERMA_ASSERT_MSG(false, "Heap slot out of bounds!");
		}

		mtx_lock(&resource_heap->imageSlotMutex);
		resource_heap->imageSlotAllocator.freeSlot(real_heap_slot);
		mtx_unlock(&resource_heap->imageSlotMutex);
	}

	auto sampler_descriptor_heap_free_sampler_slot(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot) -> void
	{
		SamplerDescriptorHeap &sampler_heap{p_ctx->samplerHeaps[p_sampler_heap]};

		mtx_lock(&sampler_heap.samplerSlotMutex);
		sampler_heap.samplerSlotAllocator.freeSlot(p_heap_slot);
		mtx_unlock(&sampler_heap.samplerSlotMutex);
	}

	auto resource_descriptor_heap_write_buffer(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, BufferHandle p_buffer) -> void
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};
		Buffer &                buffer{p_ctx->buffers[p_buffer]};

		VkDeviceAddressRangeKHR device_address_range{buffer.address, buffer.size};

		VkHostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(resource_heap.mappedHeapMemory) + static_cast<uint64>(p_heap_slot) * resource_heap.
													  bufferDescriptorSize);
		host_range.size = resource_heap.bufferDescriptorSize;

		VkDescriptorType descriptor_type{}; // I won't allow a buffer that is both uniform and storage
		if (buffer.usageFlags & EBufferUsageFlagBits::eUniformBuffer)
			descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		else if (buffer.usageFlags & EBufferUsageFlagBits::eStorageBuffer)
			descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		else
		{
			TST_PERMA_ASSERT_MSG(false, "What type of buffer is ts?!");
		}

		VkResourceDescriptorInfoEXT resource_info{};
		resource_info.sType              = STYPE(RESOURCE_DESCRIPTOR_INFO_EXT);
		resource_info.pNext              = nullptr;
		resource_info.type               = descriptor_type;
		resource_info.data.pAddressRange = &device_address_range;
		vkWriteResourceDescriptorsEXT(p_ctx->logicalDevice, 1u, &resource_info, &host_range);
	}

	auto resource_descriptor_heap_write_texture(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, TextureHandle p_texture,
												bool        p_storage, uint32                   p_mip) -> void
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		Texture *texture{p_ctx->textures.tryGet(p_texture)};
		TST_ASSERT(texture);

		const VkImageViewType image_view_type{get_vk_image_view_type(texture->desc.type)};

		VkFormat image_format{get_vk_format(texture->desc.format)};

		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType      = STYPE(IMAGE_VIEW_CREATE_INFO);
		image_view_create_info.pNext      = nullptr;
		image_view_create_info.image      = texture->image;
		image_view_create_info.viewType   = image_view_type;
		image_view_create_info.format     = image_format;
		image_view_create_info.components = VkComponentMapping{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		VkImageSubresourceRange &subresource_range{image_view_create_info.subresourceRange};
		subresource_range.aspectMask     = get_vk_image_aspect_mask(texture->desc.format);
		subresource_range.baseMipLevel   = (p_mip == UINT32_MAX) ? 0u : p_mip;
		subresource_range.levelCount     = (p_mip == UINT32_MAX) ? texture->desc.mipCount : 1u;
		subresource_range.baseArrayLayer = 0u;
		subresource_range.layerCount     = texture->desc.layerCount;

		const uint32 real_heap_slot{get_segment_relative_texture_heap_slot(p_ctx, p_resource_heap, p_heap_slot)};

		VkHostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(resource_heap.mappedHeapMemory) + resource_heap.imageSegmentOffset + static_cast<uint64>(
														  real_heap_slot) * resource_heap.imageDescriptorSize);
		host_range.size = resource_heap.imageDescriptorSize;

		VkImageDescriptorInfoEXT image_info{};
		image_info.sType  = STYPE(IMAGE_DESCRIPTOR_INFO_EXT);
		image_info.pNext  = nullptr;
		image_info.pView  = &image_view_create_info;
		image_info.layout = VK_IMAGE_LAYOUT_GENERAL;

		const VkDescriptorType descriptor_type{p_storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE};

		VkResourceDescriptorInfoEXT resource_info{};
		resource_info.sType       = STYPE(RESOURCE_DESCRIPTOR_INFO_EXT);
		resource_info.pNext       = nullptr;
		resource_info.data.pImage = &image_info;
		resource_info.type        = descriptor_type;
		vkWriteResourceDescriptorsEXT(p_ctx->logicalDevice, 1u, &resource_info, &host_range);
	}

	auto sampler_descriptor_heap_write_sampler(GPUContext *       p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot,
											   const SamplerDesc &p_sampler_desc) -> void
	{
		SamplerDescriptorHeap &sampler_heap{p_ctx->samplerHeaps[p_sampler_heap]};

		VkSamplerCreateInfo sampler_create_info{};
		sampler_create_info.sType        = STYPE(SAMPLER_CREATE_INFO);
		sampler_create_info.pNext        = nullptr;
		sampler_create_info.minFilter    = p_sampler_desc.minFilter == EFilter::eLinear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		sampler_create_info.magFilter    = p_sampler_desc.magFilter == EFilter::eLinear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		sampler_create_info.mipmapMode   = p_sampler_desc.mipmapMode == ESamplerMipmapMode::eLinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_create_info.addressModeU = get_vk_address_mode(p_sampler_desc.addressModeU);
		sampler_create_info.addressModeV = get_vk_address_mode(p_sampler_desc.addressModeV);
		sampler_create_info.addressModeW = get_vk_address_mode(p_sampler_desc.addressModeW);

		sampler_create_info.mipLodBias       = 0.0f;
		sampler_create_info.anisotropyEnable = true;

		sampler_create_info.maxAnisotropy = (p_sampler_desc.maxAnisotropy == -1.0f)
												? p_ctx->deviceProperties.properties.limits.maxSamplerAnisotropy
												: p_sampler_desc.maxAnisotropy;
		sampler_create_info.compareEnable           = false;
		sampler_create_info.compareOp               = VK_COMPARE_OP_NEVER;
		sampler_create_info.minLod                  = p_sampler_desc.minLod;
		sampler_create_info.maxLod                  = p_sampler_desc.maxLod;
		sampler_create_info.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		sampler_create_info.unnormalizedCoordinates = false;

		VkHostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(sampler_heap.mappedHeapMemory) + static_cast<uint64>(p_heap_slot) * sampler_heap.
													  samplerDescriptorSize);
		host_range.size = sampler_heap.samplerDescriptorSize;

		vkWriteSamplerDescriptorsEXT(p_ctx->logicalDevice, 1u, &sampler_create_info, &host_range);
	}

	auto buffer_create(GPUContext *p_ctx, const BufferDesc &p_desc) -> BufferHandle
	{
		const VkBufferUsageFlags buffer_usage_flags{get_vk_buffer_usage(p_desc.usage)};

		VkBufferCreateInfo buffer_create_info{};
		buffer_create_info.sType = STYPE(BUFFER_CREATE_INFO);
		buffer_create_info.pNext = nullptr;
		buffer_create_info.size  = p_desc.size;
		buffer_create_info.usage = buffer_usage_flags;

		Array<uint32, 2u> queue_family_indices{p_ctx->queueFamilyIndices.graphics, p_ctx->queueFamilyIndices.transfer};
		if (queue_family_indices[0u] != queue_family_indices[1u])
		{
			buffer_create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
			buffer_create_info.queueFamilyIndexCount = queue_family_indices.size();
			buffer_create_info.pQueueFamilyIndices   = queue_family_indices.data();
		}
		else
		{
			buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		VmaAllocationCreateInfo allocation_create_info{};
		if (p_desc.memoryType == EMemoryType::eHostVisibleCoherent)
		{
			allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		else
		{
			allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}

		VkBuffer      buffer{nullptr};
		VmaAllocation allocation{nullptr};

		VmaAllocationInfo allocation_info{};
		vmaCreateBuffer(p_ctx->allocator, reinterpret_cast<const VkBufferCreateInfo *>(&buffer_create_info), &allocation_create_info,
						reinterpret_cast<VkBuffer *>(&buffer), &allocation, &allocation_info);

		VkDeviceAddress device_address{nullGpuPtr};
		if (buffer_usage_flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo device_address_info{};
			device_address_info.sType  = STYPE(BUFFER_DEVICE_ADDRESS_INFO);
			device_address_info.pNext  = nullptr;
			device_address_info.buffer = buffer;
			device_address             = vkGetBufferDeviceAddress(p_ctx->logicalDevice, &device_address_info);
		}

		return p_ctx->buffers.emplace(Buffer{buffer, allocation, p_desc.size, device_address, allocation_info.pMappedData, p_desc.usage});
	}

	auto buffer_destroy(GPUContext *p_ctx, BufferHandle p_buffer) -> void
	{
		p_ctx->buffers.destroy(p_buffer);
	}

	auto buffer_write_data(GPUContext *p_ctx, BufferHandle p_buffer, const void *p_data, uint64 p_size, uint64 p_offset) -> void
	{
		Buffer &buffer{p_ctx->buffers[p_buffer]};
		TST_ASSERT_MSG(buffer.mapped != nullptr, "Buffer is not host visible");
		TST_ASSERT_MSG(p_offset + p_size <= buffer.size, "Buffer write exceeds allocation");
		std::memcpy(static_cast<uint8 *>(buffer.mapped) + p_offset, p_data, p_size);
	}

	auto buffer_get_mapped_data(GPUContext *p_ctx, BufferHandle p_buffer) -> void *
	{
		Buffer &buffer{p_ctx->buffers[p_buffer]};
		TST_ASSERT_MSG(buffer.mapped != nullptr, "Buffer is not host visible");
		return buffer.mapped;
	}

	auto buffer_get_address(GPUContext *p_ctx, BufferHandle p_buffer) -> DeviceAddress
	{
		Buffer &buffer{p_ctx->buffers[p_buffer]};
		return buffer.address;
	}

	auto surface_create(GPUContext *p_ctx, void *p_hwnd) -> SurfaceHandle
	{
		if (!p_ctx->usingSwapchain)
		{
			TST_PERMA_ASSERT_MSG(false, "I can only create surfaces if you are using the swapchain");
			return nullptr;
		}

		const auto hwnd{static_cast<HWND>(p_hwnd)};

		VkWin32SurfaceCreateInfoKHR surface_create_info{};
		surface_create_info.sType     = STYPE(WIN32_SURFACE_CREATE_INFO_KHR);
		surface_create_info.pNext     = nullptr;
		surface_create_info.hwnd      = hwnd;
		surface_create_info.hinstance = GetModuleHandle(nullptr);

		VkSurfaceKHR surface;
		TST_GPU_VK_CHECK_RESULT_MSG(vkCreateWin32SurfaceKHR(p_ctx->vulkanInstance, &surface_create_info, nullptr, &surface), "Failed to create window surface!");

		return p_ctx->surfaces.emplace(Surface{surface, hwnd});
	}

	auto surface_destroy(GPUContext *p_ctx, SurfaceHandle p_surface) -> void
	{
		p_ctx->surfaces.destroy(p_surface);
	}

	auto semaphore_create(GPUContext *p_ctx, uint64 p_initial_value) -> SemaphoreHandle
	{
		VkSemaphoreTypeCreateInfo timeline_semaphore_create_info{};
		timeline_semaphore_create_info.sType         = STYPE(SEMAPHORE_TYPE_CREATE_INFO);
		timeline_semaphore_create_info.pNext         = nullptr;
		timeline_semaphore_create_info.initialValue  = p_initial_value;
		timeline_semaphore_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

		VkSemaphoreCreateInfo create_info{};
		create_info.sType = STYPE(SEMAPHORE_CREATE_INFO);
		create_info.pNext = &timeline_semaphore_create_info;

		VkSemaphore semaphore;
		TST_GPU_VK_CHECK_RESULT_MSG(vkCreateSemaphore(p_ctx->logicalDevice, &create_info, nullptr, &semaphore), "Failed to create timeline semaphore");

		return p_ctx->semaphores.emplace(Semaphore{semaphore});
	}

	auto semaphore_destroy(GPUContext *p_ctx, SemaphoreHandle p_semaphore) -> void
	{
		p_ctx->semaphores.destroy(p_semaphore);
	}

	auto semaphore_wait_n(GPUContext *p_ctx, InitialiserList<const SemaphoreHandle> p_semaphores, InitialiserList<const uint64> p_values) -> void
	{
		TST_ASSERT(p_semaphores.size() == p_values.size());

		Vector<VkSemaphore> semaphores(p_semaphores.size());
		for (uint64 i{0u}; i < p_semaphores.size(); ++i)
			semaphores[i] = p_ctx->semaphores[p_semaphores[i]].semaphore;

		VkSemaphoreWaitInfo wait_info{};
		wait_info.sType          = STYPE(SEMAPHORE_WAIT_INFO);
		wait_info.pNext          = nullptr;
		wait_info.semaphoreCount = p_semaphores.size();
		wait_info.pSemaphores    = semaphores.data();
		wait_info.pValues        = p_values.data();
		wait_info.flags          = 0u;
		TST_GPU_VK_CHECK_RESULT_MSG(vkWaitSemaphores(p_ctx->logicalDevice, &wait_info, INFINITE), "Failed to wait on semaphores");
	}

	auto semaphore_get_value(GPUContext *p_ctx, SemaphoreHandle p_semaphore) -> uint64
	{
		VkSemaphore semaphore{p_ctx->semaphores[p_semaphore].semaphore};

		uint64 value;
		TST_GPU_VK_CHECK_RESULT_MSG(vkGetSemaphoreCounterValue(p_ctx->logicalDevice, semaphore, &value), "Failed to get semaphore counter value");

		return value;
	}

	auto get_num_undefined_textures(GPUContext *p_ctx) -> uint64
	{
		mtx_lock(&p_ctx->undefinedTexturesMutex);
		const uint64 count{p_ctx->undefinedTextures.size()};
		mtx_unlock(&p_ctx->undefinedTexturesMutex);
		return count;
	}

	auto texture_create(GPUContext *p_ctx, const TextureDesc &p_desc) -> TextureHandle
	{
		const VkImageType           image_type{get_vk_image_type(p_desc.type)};
		const VkImageViewType       image_view_type{get_vk_image_view_type(p_desc.type)};
		const VkImageCreateFlags    image_create_flags{p_desc.type == ETextureType::eCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u};
		const VkFormat              image_format{get_vk_format(p_desc.format)};
		const VkImageUsageFlags     image_usage_flags{get_vk_image_usage(p_desc.usage)};
		const VkSampleCountFlagBits sample_count{get_vk_sample_count(p_desc.sampleCount)};

		VkImageCreateInfo image_create_info{};
		image_create_info.sType         = STYPE(IMAGE_CREATE_INFO);
		image_create_info.pNext         = nullptr;
		image_create_info.flags         = image_create_flags;
		image_create_info.imageType     = image_type;
		image_create_info.format        = image_format;
		image_create_info.extent        = VkExtent3D{p_desc.extent.x, p_desc.extent.y, p_desc.extent.z};
		image_create_info.mipLevels     = p_desc.mipCount;
		image_create_info.arrayLayers   = p_desc.layerCount;
		image_create_info.samples       = sample_count;
		image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage         = image_usage_flags;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		Array<uint32, 2u> queue_family_indices{p_ctx->queueFamilyIndices.graphics, p_ctx->queueFamilyIndices.transfer}; // Again, I am not doin allat
		if (queue_family_indices[0u] != queue_family_indices[1u])
		{
			image_create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
			image_create_info.queueFamilyIndexCount = queue_family_indices.size();
			image_create_info.pQueueFamilyIndices   = queue_family_indices.data();
		}
		else
		{
			image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		VkImage       image{nullptr};
		VkImageView   image_view{nullptr};
		VmaAllocation allocation{nullptr};

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

		vmaCreateImage(p_ctx->allocator, &image_create_info, &allocation_create_info, &image, &allocation, nullptr);

		// If the texture is a render target, it needs an image view
		if (p_desc.usage & ETextureUsageFlagBits::eColourAttachment || p_desc.usage & ETextureUsageFlagBits::eDepthStencilAttachment)
		{
			VkImageViewCreateInfo image_view_create_info{};
			image_view_create_info.sType      = STYPE(IMAGE_VIEW_CREATE_INFO);
			image_view_create_info.pNext      = nullptr;
			image_view_create_info.image      = image;
			image_view_create_info.viewType   = image_view_type;
			image_view_create_info.format     = image_format;
			image_view_create_info.components = VkComponentMapping{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};
			VkImageSubresourceRange &subresource_range{image_view_create_info.subresourceRange};
			subresource_range.aspectMask     = get_vk_image_aspect_mask(p_desc.format);
			subresource_range.baseMipLevel   = 0u;
			subresource_range.levelCount     = p_desc.mipCount;
			subresource_range.baseArrayLayer = 0u;
			subresource_range.layerCount     = p_desc.layerCount;

			TST_GPU_VK_CHECK_RESULT_MSG(vkCreateImageView(p_ctx->logicalDevice, &image_view_create_info, nullptr, &image_view), "Failed to create texture's image view");
		}

		const TextureHandle out_texture{p_ctx->textures.emplace(Texture{image, image_view, allocation, p_desc, false})};

		mtx_lock(&p_ctx->undefinedTexturesMutex);
		p_ctx->undefinedTextures.insert(out_texture);
		mtx_unlock(&p_ctx->undefinedTexturesMutex);

		return out_texture;
	}

	auto texture_destroy(GPUContext *p_ctx, TextureHandle p_texture) -> void
	{
		p_ctx->textures.destroy(p_texture);

		mtx_lock(&p_ctx->undefinedTexturesMutex);
		p_ctx->undefinedTextures.erase(p_texture);
		mtx_unlock(&p_ctx->undefinedTexturesMutex);
	}

	auto texture_get_desc(GPUContext *p_ctx, TextureHandle p_texture) -> const TextureDesc &
	{
		const Texture &texture{p_ctx->textures[p_texture]};
		return texture.desc;
	}

	static auto create_swapchain_objects(GPUContext *p_ctx, Swapchain &p_swapchain, math::UInt2 p_extent, VkSwapchainKHR p_old_swapchain = nullptr) -> bool
	{
		VkSurfaceKHR surface{p_ctx->surfaces[p_swapchain.surface].surface};

		VkSurfaceCapabilitiesKHR surface_capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_ctx->physicalDevice, surface, &surface_capabilities);

		uint32 available_format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(p_ctx->physicalDevice, surface, &available_format_count, nullptr);
		if (available_format_count == 0u)
		{
			TST_PERMA_ASSERT_MSG(false, "Window surface has no available formats!");
			return false;
		}
		Vector<VkSurfaceFormatKHR> available_formats(available_format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(p_ctx->physicalDevice, surface, &available_format_count, available_formats.data());

		// According to my expert research, the most aesthetically pleasing image format is RGBA in the SRGB colour space.
		// If for some reason, your GPU does not support that, then just fall back to the first available format.
		const auto format_it = std::ranges::find_if(available_formats, +[](const VkSurfaceFormatKHR &p_format)
		{
			return p_format.format == VK_FORMAT_R8G8B8A8_SRGB && p_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		});
		VkSurfaceFormatKHR surface_format{format_it == available_formats.end() ? available_formats[0] : *format_it};

		// Ideally, we want the min image count to be at least 3. However, if your GPU is bad, it might not be able to handle that many images.
		// So if 3 is greater than the max image count, we fall back to the max image count as the min image count... I don't know if that made sense...
		uint32 min_image_count{std::max(3u, surface_capabilities.minImageCount)};

		// Apparently, if the maxImageCount == 0, then there is no maximum (unlimited).
		if ((surface_capabilities.maxImageCount > 0) && (surface_capabilities.maxImageCount < min_image_count))
			min_image_count = surface_capabilities.maxImageCount;

		uint32 available_present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(p_ctx->physicalDevice, surface, &available_present_mode_count, nullptr);
		if (available_present_mode_count == 0u)
		{
			TST_PERMA_ASSERT_MSG(false, "Window surface has no available present modes!");
			return false;
		}
		Vector<VkPresentModeKHR> available_present_modes(available_present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(p_ctx->physicalDevice, surface, &available_present_mode_count, available_present_modes.data());

		// The ideal present mode would be mailbox because it is not limited to the refresh rate of the monitor.
		// However, not every device supports it. But Fifo is guaranteed to be supported, so that is the fallback option.
		// Also, when using Mailbox, as there is no default framerate cap, your GPU usage may go to 100%. This is why I am using vk_NV_low_latency2.
		VkPresentModeKHR present_mode{
			std::ranges::any_of(available_present_modes, +[](const VkPresentModeKHR p_present_mode)
			{
				return p_present_mode == VK_PRESENT_MODE_MAILBOX_KHR;
			})
				? VK_PRESENT_MODE_MAILBOX_KHR
				: VK_PRESENT_MODE_FIFO_KHR
		};

		p_swapchain.surfaceFormat = surface_format;
		p_swapchain.minImageCount = min_image_count;
		p_swapchain.presentMode   = present_mode;

		VkExtent2D swapchain_extent{};

		// If the current extent is UINT32_MAX, it means that we can choose our own custom extent
		if (surface_capabilities.currentExtent.width != UINT32_MAX)
			swapchain_extent = surface_capabilities.currentExtent;

		// But we still have to make sure that we clamp the extent between the min and max.
		// I think this probably has to do with certain displays (Apple Retina) having a very high pixel density (DPI).
		swapchain_extent = VkExtent2D{
			std::clamp<uint32>(p_extent.x, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
			std::clamp<uint32>(p_extent.y, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height)
		};

		if (swapchain_extent.width == 0u || swapchain_extent.height == 0u) // If the window size is [0, 0], we cannot create a valid swapchain
			return false;

		// Allow the swapchain to use Nvidia low latency mode.
		VkSwapchainLatencyCreateInfoNV swapchain_latency_create_info{};
		swapchain_latency_create_info.sType             = STYPE(SWAPCHAIN_LATENCY_CREATE_INFO_NV);
		swapchain_latency_create_info.pNext             = nullptr;
		swapchain_latency_create_info.latencyModeEnable = true;

		VkSwapchainCreateInfoKHR swapchain_create_info{};
		swapchain_create_info.sType            = STYPE(SWAPCHAIN_CREATE_INFO_KHR);
		swapchain_create_info.pNext            = &swapchain_latency_create_info;
		swapchain_create_info.surface          = surface;
		swapchain_create_info.minImageCount    = p_swapchain.minImageCount;
		swapchain_create_info.imageFormat      = p_swapchain.surfaceFormat.format;
		swapchain_create_info.imageColorSpace  = p_swapchain.surfaceFormat.colorSpace;
		swapchain_create_info.imageExtent      = swapchain_extent;
		swapchain_create_info.imageArrayLayers = 1u;
		swapchain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_create_info.preTransform     = surface_capabilities.currentTransform;
		swapchain_create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.presentMode      = p_swapchain.presentMode;
		swapchain_create_info.clipped          = true;
		swapchain_create_info.oldSwapchain     = p_old_swapchain; // Performance improvement?

		if (vkCreateSwapchainKHR(p_ctx->logicalDevice, &swapchain_create_info, nullptr, &p_swapchain.swapchain) != VK_SUCCESS)
		{
			p_swapchain.swapchain = nullptr;
			return false;
		}

		p_swapchain.extent = {swapchain_extent.width, swapchain_extent.height};

		TextureDesc attachment_desc{};
		attachment_desc.type        = ETextureType::e2D;
		attachment_desc.extent      = {p_swapchain.extent.x, p_swapchain.extent.y, 1u};
		attachment_desc.mipCount    = 1u;
		attachment_desc.layerCount  = 1u; // What kind of swapchain has multiple layers?!
		attachment_desc.sampleCount = ESampleCount::e1;
		attachment_desc.format      = get_tst_format(p_swapchain.surfaceFormat.format);
		attachment_desc.usage       = ETextureUsageFlagBits::eColourAttachment;

		uint32 swapchain_image_count;
		vkGetSwapchainImagesKHR(p_ctx->logicalDevice, p_swapchain.swapchain, &swapchain_image_count, nullptr);
		TST_ASSERT_MSG(swapchain_image_count != 0u, "The swapchain apparently has 0 images?!");
		Vector<VkImage> swapchain_images(swapchain_image_count);
		vkGetSwapchainImagesKHR(p_ctx->logicalDevice, p_swapchain.swapchain, &swapchain_image_count, swapchain_images.data());

		p_swapchain.attachments.resize(swapchain_image_count);
		for (uint32 i{0u}; i < swapchain_image_count; ++i)
		{
			const VkImage swapchain_image{swapchain_images[i]};

			VkImageViewCreateInfo image_view_create_info{};
			image_view_create_info.sType      = STYPE(IMAGE_VIEW_CREATE_INFO);
			image_view_create_info.pNext      = nullptr;
			image_view_create_info.image      = swapchain_image;
			image_view_create_info.viewType   = VK_IMAGE_VIEW_TYPE_2D;
			image_view_create_info.format     = p_swapchain.surfaceFormat.format;
			image_view_create_info.components = VkComponentMapping{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};

			VkImageSubresourceRange &subresource_range{image_view_create_info.subresourceRange};
			subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource_range.baseMipLevel   = 0u;
			subresource_range.levelCount     = 1u;
			subresource_range.baseArrayLayer = 0u;
			subresource_range.layerCount     = 1u;

			VkImageView image_view;
			TST_GPU_VK_CHECK_RESULT_MSG(vkCreateImageView(p_ctx->logicalDevice, &image_view_create_info, nullptr, &image_view), "Failed to create swapchain image view!");

			// I don't know why I am providing this, as there is no reason to do so. But I guess now you can query the swapchain's texture desc?...
			TextureDesc fake_desc{
				math::UInt3{p_swapchain.extent.x, p_swapchain.extent.y, 1u},
				1u,
				1u,
				ETextureType::e2D,
				ESampleCount::e1,
				get_tst_format(p_swapchain.surfaceFormat.format),
				ETextureUsageFlagBits::eNone
			};
			p_swapchain.attachments[i] = p_ctx->textures.emplace(Texture{swapchain_image, image_view, nullptr, fake_desc, true});
		}

		p_swapchain.renderFinishedSemaphores.resize(swapchain_image_count);
		static constexpr VkSemaphoreCreateInfo c_semaphore_create_info{STYPE(SEMAPHORE_CREATE_INFO), nullptr, 0u};
		for (auto &semaphore: p_swapchain.renderFinishedSemaphores)
			vkCreateSemaphore(p_ctx->logicalDevice, &c_semaphore_create_info, nullptr, &semaphore);

		return true;
	}

	static auto destroy_swapchain_objects(GPUContext *p_ctx, Swapchain &p_swapchain) -> void
	{
		for (auto &attachment: p_swapchain.attachments)
			p_ctx->textures.destroy(attachment);
		p_swapchain.attachments.clear();

		for (auto &semaphore: p_swapchain.renderFinishedSemaphores)
			vkDestroySemaphore(p_ctx->logicalDevice, semaphore, nullptr);
		p_swapchain.renderFinishedSemaphores.clear();
	}

	auto swapchain_create(GPUContext *p_ctx, SurfaceHandle p_surface, math::UInt2 p_initial_extent) -> SwapchainHandle
	{
		Swapchain swapchain{};
		swapchain.surface = p_surface;
		swapchain.extent  = p_initial_extent;

		swapchain.latencySemaphore = semaphore_create(p_ctx, 0ull);

		swapchain.imageAvailableSemaphores.resize(p_ctx->maxConcurrentSwapchainWorkloads);
		static constexpr VkSemaphoreCreateInfo c_semaphore_create_info{STYPE(SEMAPHORE_CREATE_INFO), nullptr, 0u};
		for (auto &semaphore: swapchain.imageAvailableSemaphores)
			vkCreateSemaphore(p_ctx->logicalDevice, &c_semaphore_create_info, nullptr, &semaphore);

		create_swapchain_objects(p_ctx, swapchain, p_initial_extent);

		return p_ctx->swapchains.emplace(std::move(swapchain));
	}

	auto swapchain_resize(GPUContext *p_ctx, SwapchainHandle p_swapchain, math::UInt2 p_extent) -> bool
	{
		vkDeviceWaitIdle(p_ctx->logicalDevice);

		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};

		VkSwapchainKHR old_swapchain{swapchain.swapchain};

		destroy_swapchain_objects(p_ctx, swapchain);
		if (!create_swapchain_objects(p_ctx, swapchain, p_extent, old_swapchain)) // You can handle ts on your own
			return false;

		if (old_swapchain)
			vkDestroySwapchainKHR(p_ctx->logicalDevice, old_swapchain, nullptr);

		return true;
	}

	auto swapchain_destroy(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> void
	{
		p_ctx->swapchains.destroy(p_swapchain);
	}

	auto swapchain_set_max_framerate(GPUContext *p_ctx, SwapchainHandle p_swapchain, uint32 p_framerate) -> void
	{
		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};

		VkLatencySleepModeInfoNV sleep_mode_info{};
		sleep_mode_info.sType           = STYPE(LATENCY_SLEEP_MODE_INFO_NV);
		sleep_mode_info.pNext           = nullptr;
		sleep_mode_info.lowLatencyMode  = true;
		sleep_mode_info.lowLatencyBoost = true;

		if (p_framerate == UINT32_MAX)
			sleep_mode_info.minimumIntervalUs = 0u;
		else
			sleep_mode_info.minimumIntervalUs = static_cast<uint32>(1'000'000.0 / static_cast<float64>(p_framerate));

		TST_GPU_VK_CHECK_RESULT_MSG(vkSetLatencySleepModeNV(p_ctx->logicalDevice, swapchain.swapchain, &sleep_mode_info), "Failed to set latency sleep mode!");
	}

	auto swapchain_wait_for_framerate(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> void
	{
		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};
		Semaphore &latency_semaphore{p_ctx->semaphores[swapchain.latencySemaphore]};

		VkLatencySleepInfoNV sleep_info{};
		sleep_info.sType           = STYPE(LATENCY_SLEEP_INFO_NV);
		sleep_info.pNext           = nullptr;
		sleep_info.value           = swapchain.frameId;
		sleep_info.signalSemaphore = latency_semaphore.semaphore;
		TST_GPU_VK_CHECK_RESULT_MSG_DEBUG(vkLatencySleepNV(p_ctx->logicalDevice, swapchain.swapchain, &sleep_info), "Failed to latency sleep!");

		semaphore_wait_n(p_ctx, swapchain.latencySemaphore, swapchain.frameId);
	}

	auto swapchain_set_frame_marker(GPUContext *p_ctx, SwapchainHandle p_swapchain, EFrameMarker p_marker) -> void
	{
		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};

		VkSetLatencyMarkerInfoNV latency_marker{};
		latency_marker.sType     = STYPE(SET_LATENCY_MARKER_INFO_NV);
		latency_marker.pNext     = nullptr;
		latency_marker.presentID = swapchain.frameId;
		latency_marker.marker    = get_vk_nv_latency_marker(p_marker);

		vkSetLatencyMarkerNV(p_ctx->logicalDevice, swapchain.swapchain, &latency_marker);
	}

	auto swapchain_acquire_next_image(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> TextureHandle
	{
		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};
		if (!swapchain.swapchain)
			return nullptr;

		swapchain.acquisitonIndex = (swapchain.acquisitonIndex + 1u) % swapchain.imageAvailableSemaphores.size();

		VkAcquireNextImageInfoKHR acquire_info{};
		acquire_info.sType      = STYPE(ACQUIRE_NEXT_IMAGE_INFO_KHR);
		acquire_info.pNext      = nullptr;
		acquire_info.swapchain  = swapchain.swapchain;
		acquire_info.timeout    = INFINITE;
		acquire_info.semaphore  = swapchain.imageAvailableSemaphores[swapchain.acquisitonIndex];
		acquire_info.fence      = nullptr;
		acquire_info.deviceMask = 1u;
		if (vkAcquireNextImage2KHR(p_ctx->logicalDevice, &acquire_info, &swapchain.imageIndex) != VK_SUCCESS)
			return nullptr;

		return swapchain.attachments[swapchain.imageIndex];
	}

	auto swapchain_insert_pre_render_resource_barrier(GPUContext *p_ctx, CommandListHandle p_cmd_list, TextureHandle p_attachment_texture) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Texture &attachment{p_ctx->textures[p_attachment_texture]};

		VkImageMemoryBarrier2 undefined_to_general{};
		undefined_to_general.sType               = STYPE(IMAGE_MEMORY_BARRIER_2);
		undefined_to_general.pNext               = nullptr;
		undefined_to_general.image               = attachment.image;
		undefined_to_general.srcAccessMask       = VK_ACCESS_2_NONE;
		undefined_to_general.dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		undefined_to_general.srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		undefined_to_general.dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		undefined_to_general.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		undefined_to_general.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		undefined_to_general.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
		undefined_to_general.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
		VkImageSubresourceRange &subresource_range{undefined_to_general.subresourceRange};
		subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel   = 0u;
		subresource_range.levelCount     = 1u;
		subresource_range.baseArrayLayer = 0u;
		subresource_range.layerCount     = 1u;

		VkDependencyInfo dependency_info{};
		dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
		dependency_info.pNext                    = nullptr;
		dependency_info.imageMemoryBarrierCount  = 1u;
		dependency_info.pImageMemoryBarriers     = &undefined_to_general;
		dependency_info.bufferMemoryBarrierCount = 0u;
		dependency_info.memoryBarrierCount       = 0u;

		vkCmdPipelineBarrier2(cmd.cmd, &dependency_info);
	}

	auto swapchain_submit_and_present(GPUContext *               p_ctx, SwapchainHandle p_swapchain, CommandListHandle p_cmd_list,
									  const SemaphoreSubmitInfo &p_signal_semaphore_info) -> bool
	{
		return swapchain_submit_and_present(p_ctx, p_swapchain, p_cmd_list, p_signal_semaphore_info, {});
	}

	auto swapchain_submit_and_present(GPUContext *p_ctx, SwapchainHandle p_swapchain, CommandListHandle p_cmd_list, const SemaphoreSubmitInfo &p_signal_semaphore_info,
									  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos) -> bool
	{
		Swapchain &  swapchain{p_ctx->swapchains[p_swapchain]};
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Texture &colour_attachment{p_ctx->textures[swapchain.attachments[swapchain.imageIndex]]};

		VkImageMemoryBarrier2 general_to_present_src{};
		general_to_present_src.sType               = STYPE(IMAGE_MEMORY_BARRIER_2);
		general_to_present_src.pNext               = nullptr;
		general_to_present_src.image               = colour_attachment.image;
		general_to_present_src.srcAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		general_to_present_src.dstAccessMask       = VK_ACCESS_2_NONE;
		general_to_present_src.srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		general_to_present_src.dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		general_to_present_src.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		general_to_present_src.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		general_to_present_src.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
		general_to_present_src.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		VkImageSubresourceRange &subresource_range{general_to_present_src.subresourceRange};
		subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel   = 0u;
		subresource_range.levelCount     = 1u;
		subresource_range.baseArrayLayer = 0u;
		subresource_range.layerCount     = 1u;

		VkDependencyInfo dependency_info{};
		dependency_info.sType                    = STYPE(DEPENDENCY_INFO);
		dependency_info.pNext                    = nullptr;
		dependency_info.imageMemoryBarrierCount  = 1u;
		dependency_info.pImageMemoryBarriers     = &general_to_present_src;
		dependency_info.bufferMemoryBarrierCount = 0u;
		dependency_info.memoryBarrierCount       = 0u;

		vkCmdPipelineBarrier2(cmd.cmd, &dependency_info);

		cmd_list_close(p_ctx, p_cmd_list);

		Vector<VkSemaphoreSubmitInfo> wait_infos;
		wait_infos.reserve(1u + p_wait_semaphore_infos.size());

		VkSemaphoreSubmitInfo &image_available_submit_info{wait_infos.emplace_back()};
		image_available_submit_info.sType       = STYPE(SEMAPHORE_SUBMIT_INFO);
		image_available_submit_info.pNext       = nullptr;
		image_available_submit_info.semaphore   = swapchain.imageAvailableSemaphores[swapchain.acquisitonIndex];
		image_available_submit_info.value       = 0ull; // Binary semaphore
		image_available_submit_info.stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		image_available_submit_info.deviceIndex = 0u;

		for (const SemaphoreSubmitInfo &wait: p_wait_semaphore_infos)
		{
			VkSemaphoreSubmitInfo &wait_info{wait_infos.emplace_back()};
			wait_info.sType       = STYPE(SEMAPHORE_SUBMIT_INFO);
			wait_info.pNext       = nullptr;
			wait_info.semaphore   = p_ctx->semaphores[wait.semaphore].semaphore;
			wait_info.value       = wait.value; // Timeline semaphore
			wait_info.stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			wait_info.deviceIndex = 0u;
		}

		VkSemaphore render_finished_semaphore{swapchain.renderFinishedSemaphores[swapchain.imageIndex]};

		Vector<VkSemaphoreSubmitInfo> signal_infos;
		signal_infos.reserve(2u);

		VkSemaphoreSubmitInfo &render_finished_submit_info{signal_infos.emplace_back()};
		render_finished_submit_info.sType       = STYPE(SEMAPHORE_SUBMIT_INFO);
		render_finished_submit_info.pNext       = nullptr;
		render_finished_submit_info.semaphore   = render_finished_semaphore;
		render_finished_submit_info.value       = 0ull; // Binary semaphore
		render_finished_submit_info.stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		render_finished_submit_info.deviceIndex = 0u;

		if (p_signal_semaphore_info.semaphore)
		{
			VkSemaphoreSubmitInfo &custom_submit_info{signal_infos.emplace_back()};
			custom_submit_info.sType       = STYPE(SEMAPHORE_SUBMIT_INFO);
			custom_submit_info.pNext       = nullptr;
			custom_submit_info.semaphore   = p_ctx->semaphores[p_signal_semaphore_info.semaphore].semaphore;
			custom_submit_info.value       = p_signal_semaphore_info.value; // Timeline semaphore
			custom_submit_info.stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			custom_submit_info.deviceIndex = 0u;
		}

		submit(p_ctx, EQueueType::eGraphics, p_cmd_list, wait_infos, signal_infos);

		VkLatencySubmissionPresentIdNV present_id_info{};
		present_id_info.sType     = STYPE(LATENCY_SUBMISSION_PRESENT_ID_NV);
		present_id_info.pNext     = nullptr;
		present_id_info.presentID = swapchain.frameId;

		VkPresentInfoKHR present_info{};
		present_info.sType              = STYPE(PRESENT_INFO_KHR);
		present_info.pNext              = &present_id_info;
		present_info.waitSemaphoreCount = 1u; // Render finished semaphore
		present_info.pWaitSemaphores    = &render_finished_semaphore;
		present_info.swapchainCount     = 1u;
		present_info.pSwapchains        = &swapchain.swapchain;
		present_info.pImageIndices      = &swapchain.imageIndex;
		present_info.pResults           = nullptr;

		const VkResult result{vkQueuePresentKHR(p_ctx->queues[static_cast<uint32>(EQueueType::eGraphics)], &present_info)};

		++swapchain.frameId;

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			return false;
		else if (result != VK_SUCCESS)
			TST_PERMA_ASSERT_MSG(false, "Failed to present swapchain image!");

		return true;
	}

	auto shader_create(GPUContext *p_ctx, const ShaderDesc &p_desc) -> ShaderHandle
	{
		const VkShaderStageFlagBits shader_stage{get_vk_shader_stage(p_desc.stage)};
		const VkShaderStageFlags    next_stage{get_vk_shader_stages(p_desc.nextStage)};

		TST_PERMA_ASSERT(shader_stage != 0u);

		VkShaderCreateInfoEXT shader_create_info{};
		shader_create_info.sType = STYPE(SHADER_CREATE_INFO_EXT);
		shader_create_info.pNext = nullptr;
		shader_create_info.flags = VK_SHADER_CREATE_DESCRIPTOR_HEAP_BIT_EXT | ((p_desc.noTaskShader && shader_stage == VK_SHADER_STAGE_MESH_BIT_EXT)
																				   ? VK_SHADER_CREATE_NO_TASK_SHADER_BIT_EXT
																				   : 0u);
		shader_create_info.stage     = shader_stage;
		shader_create_info.nextStage = next_stage;
		shader_create_info.codeType  = VK_SHADER_CODE_TYPE_SPIRV_EXT;
		shader_create_info.codeSize  = p_desc.codeSizeWords * sizeof(uint32);
		shader_create_info.pCode     = p_desc.code;
		shader_create_info.pName     = p_desc.entryPointName;

		VkShaderEXT shader;
		TST_GPU_VK_CHECK_RESULT_MSG(vkCreateShadersEXT(p_ctx->logicalDevice, 1u, &shader_create_info, nullptr, &shader), "Failed to create shader");

		return p_ctx->shaders.emplace(Shader{shader, shader_stage, next_stage});
	}

	auto shader_destroy(GPUContext *p_ctx, ShaderHandle p_shader) -> void
	{
		p_ctx->shaders.destroy(p_shader);
	}
}
