#include "toast_gpu/api.hpp"

#include "toast_lib/pool.hpp"
#include "toast_lib/freelist_allocator.hpp"

#include <unordered_set>
#include <algorithm>
#include <ranges>

#include <threads.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include <vma/vk_mem_alloc.h>

#undef min
#undef max

namespace toaster::gpu
{
	using ExtensionSet = std::unordered_set<CString>;

	class FunctionDispatcher
	{
	public:
		// Call before creating the instance
		static auto initBaseFunctions() -> void
		{
			s_dldy.init();
		}

		// Call right after creating the instance
		static auto initInstanceFunctions(vk::Instance p_instance) -> void
		{
			s_dldy.init(p_instance);
		}

		// Call right after creating the logical device
		static auto initDeviceFunctions(vk::Device p_device) -> void
		{
			s_dldy.init(p_device);
		}

		static auto get() -> const vk::detail::DispatchLoaderDynamic & { return s_dldy; }

	private:
		static inline vk::detail::DispatchLoaderDynamic s_dldy{};
	};

	struct QueueFamilyIndices
	{
		[[nodiscard]] constexpr auto get(EQueueType p_queue_type) const -> uint32
		{
			switch (p_queue_type)
			{
				case EQueueType::eGraphics: return graphics;
				case EQueueType::eCompute: return compute;
				case EQueueType::eTransfer: return transfer;
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

		vk::BindHeapInfoEXT bindInfo{};

		vk::Buffer    heapBuffer{nullptr};
		VmaAllocation heapAllocation{nullptr};
		void *        mappedHeapMemory{nullptr};

		FreelistAllocator<uint32> bufferSlotAllocator;
		FreelistAllocator<uint32> imageSlotAllocator;

		vk::DeviceSize bufferDescriptorSize{0u};
		vk::DeviceSize imageDescriptorSize{0u};

		vk::DeviceSize bufferSegmentSize{0u};

		uint64         imageSegmentOffset{0u};
		vk::DeviceSize imageSegmentSize{0u};
	};

	struct SamplerDescriptorHeap
	{
		mtx_t samplerSlotMutex{};

		vk::BindHeapInfoEXT bindInfo{};

		vk::Buffer    heapBuffer{nullptr};
		VmaAllocation heapAllocation{nullptr};
		void *        mappedHeapMemory{nullptr};

		FreelistAllocator<uint32> samplerSlotAllocator;

		vk::DeviceSize samplerDescriptorSize{0u};
	};

	struct CommandList
	{
		vk::CommandBuffer cmd{nullptr};
		vk::CommandPool   pool{nullptr};

		EQueueType queueType{EQueueType::eGraphics};

		bool open{false};
		bool secondary{false};
	};

	struct Semaphore
	{
		vk::Semaphore semaphore{nullptr};
	};

	struct Buffer
	{
		vk::Buffer    buffer{nullptr};
		VmaAllocation allocation{nullptr};
		uint64        size{0u};

		DeviceAddress     address{0u};
		void *            mapped{nullptr};
		EBufferUsageFlags usageFlags{0u};
	};

	struct Texture
	{
		vk::Image     image{nullptr};
		vk::ImageView imageView{nullptr}; // Not necessary unless the texture is a render target
		VmaAllocation allocation{nullptr};

		TextureDesc desc; // Useful

		bool isSwapchainImage{false}; // Used to know whether the texture owns it's image and should create/destroy it
	};

	struct Sampler
	{
		SamplerDesc desc{};
	};

	struct Surface
	{
		vk::SurfaceKHR surface{nullptr};
		HWND           hwnd{nullptr};
	};

	struct Swapchain
	{
		std::vector<TextureHandle> attachments;
		std::vector<vk::Semaphore> imageAvailableSemaphores; // The max number of concurrent gpu swapchain workloads
		std::vector<vk::Semaphore> renderFinishedSemaphores; // PSI

		vk::SwapchainKHR swapchain{nullptr};
		SurfaceHandle    surface{nullptr};
		math::UInt2      extent{0u, 0u};

		uint32 acquisitonIndex{0u}; // This can be different from the frame index. But 99% of the time it will be the same
		uint32 imageIndex{0u};

		vk::SurfaceFormatKHR surfaceFormat{};
		uint32               minImageCount{0u};
		vk::PresentModeKHR   presentMode{vk::PresentModeKHR::eFifo};
	};

	struct Shader
	{
		vk::ShaderEXT shader{nullptr};

		vk::ShaderStageFlagBits stage{0u};
		vk::ShaderStageFlags    nextStage{0u};
	};

	#define TST_REGISTER_RESOURCE_POOL(__resourceType, __name) struct  Pool<__resourceType>  __name

	struct GPUContext
	{
		bool usingSwapchain{false}; // From the desc

		uint32 maxConcurrentSwapchainWorkloads{3u};

		vk::Instance vulkanInstance{nullptr};

		#pragma region physical device

		vk::PhysicalDevice                            physicalDevice{nullptr};
		vk::PhysicalDeviceProperties2                 deviceProperties;
		vk::PhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties;

		#pragma endregion

		#pragma region logical device

		vk::Device logicalDevice{nullptr};

		QueueFamilyIndices        queueFamilyIndices{};
		std::array<vk::Queue, 3u> queues;
		mtx_t                     queueMutexes[3u];

		#pragma endregion

		VmaAllocator allocator{nullptr};

		TST_REGISTER_RESOURCE_POOL(CommandList, commandLists);
		mtx_t                                          freeCommandListMutex;
		std::array<std::vector<CommandListHandle>, 3u> freeCommandLists;   // Free command lists per queue type
		std::array<std::vector<CommandListHandle>, 3u> freeSecondaryLists; // Free command lists per queue type
		std::array<vk::CommandPool, 3u>                transientPools;     // Mostly used for allocating command buffers for image layout transitions

		TST_REGISTER_RESOURCE_POOL(ResourceDescriptorHeap, resourceHeaps);
		TST_REGISTER_RESOURCE_POOL(SamplerDescriptorHeap, samplerHeaps);

		TST_REGISTER_RESOURCE_POOL(Texture, textures);
		std::unordered_set<TextureHandle> undefinedTextures; // All the textures that are in the undefined format and are awaiting a layout transition
		mtx_t                             undefinedTexturesMutex;

		TST_REGISTER_RESOURCE_POOL(Semaphore, semaphores);
		TST_REGISTER_RESOURCE_POOL(Buffer, buffers);
		TST_REGISTER_RESOURCE_POOL(Sampler, samplers);
		TST_REGISTER_RESOURCE_POOL(Surface, surfaces);
		TST_REGISTER_RESOURCE_POOL(Swapchain, swapchains);
		TST_REGISTER_RESOURCE_POOL(Shader, shaders);
	};

	// static APIImpl *p_ctx{nullptr};

	static auto defaultDebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      p_message_severity, vk::DebugUtilsMessageTypeFlagsEXT p_message_type,
									 const vk::DebugUtilsMessengerCallbackDataEXT *p_callback_data, [[maybe_unused]] void *              p_user_data) -> vk::Bool32
	{
		switch (p_message_severity)
		{
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
				std::printf("[Verbose] | Validation layer: %s | Message: %s", vk::to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
				std::printf("[Info] | Validation layer: %s | Message: %s", vk::to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
				std::printf("[Warning] | Validation layer: %s | Message: %s", vk::to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
				std::printf("[Error] | Validation layer: %s | Message: %s", vk::to_string(p_message_type).c_str(), p_callback_data->pMessage);
				break;
			default: break;
		}
		return vk::False;
	}

	using DeviceSuitabilityFn = bool(*)(vk::PhysicalDevice);
	// Checks the physical device against the baseline required suitability standards and optionally a custom function
	auto isDeviceSuitable(vk::PhysicalDevice  p_physical_device, const ExtensionSet &p_required_extensions,
						  DeviceSuitabilityFn p_extra_suitability_check = nullptr) -> bool
	{
		const auto props{p_physical_device.getProperties()};
		const bool vulkan_1_4_support{props.apiVersion >= vk::ApiVersion14};

		auto       queue_family_props{p_physical_device.getQueueFamilyProperties()};
		const bool supports_graphics{
			std::ranges::any_of(queue_family_props, [](const auto &queue_family)
			{
				return !!(queue_family.queueFlags & vk::QueueFlagBits::eGraphics);
			})
		};

		const bool supports_compute{
			std::ranges::any_of(queue_family_props, [](const auto &queue_family)
			{
				return !!(queue_family.queueFlags & vk::QueueFlagBits::eCompute);
			})
		};

		// Checks if all the required extensions are present in the available_device_extensions vector.
		auto       available_device_extensions{p_physical_device.enumerateDeviceExtensionProperties()};
		const bool supports_all_required_device_extensions{
			std::ranges::all_of(p_required_extensions, [available_device_extensions](const auto &required_ext)
			{
				return std::ranges::any_of(available_device_extensions, [&required_ext](const auto &available_ext)
				{
					return std::strcmp(available_ext.extensionName, required_ext) == 0;
				});
			})
		};

		const bool passes_additional_check{p_extra_suitability_check ? p_extra_suitability_check(p_physical_device) : true};

		return vulkan_1_4_support && supports_graphics && supports_compute && supports_all_required_device_extensions && passes_additional_check;
	}

	auto selectQueueFamilyIndices(vk::PhysicalDevice p_physical_device, bool p_use_present) -> QueueFamilyIndices
	{
		const auto queue_family_props{p_physical_device.getQueueFamilyProperties()};

		QueueFamilyIndices queue_family_indices{};

		for (uint32 i{0u}; i < queue_family_props.size(); ++i)
		{
			const auto queue_flags{queue_family_props[i].queueFlags};
			if (queue_flags & vk::QueueFlagBits::eGraphics && queue_flags & vk::QueueFlagBits::eCompute && (p_use_present
																												? p_physical_device.getWin32PresentationSupportKHR(i)
																												: true))
			{
				queue_family_indices.graphics = i;
				queue_family_indices.compute  = i;
			}

			if (queue_flags & vk::QueueFlagBits::eTransfer && !(queue_flags & vk::QueueFlagBits::eGraphics) && !(queue_flags & vk::QueueFlagBits::eCompute))
			{
				queue_family_indices.transfer = i;
			}
		}

		return queue_family_indices;
	}

	constexpr auto getVulkanFormat(EFormat p_format) -> vk::Format
	{
		switch (p_format)
		{
			case EFormat::eUndefined: return vk::Format::eUndefined;
			case EFormat::eR8Unorm: return vk::Format::eR8Unorm;
			case EFormat::eR8Snorm: return vk::Format::eR8Snorm;
			case EFormat::eR8Uint: return vk::Format::eR8Uint;
			case EFormat::eR8Sint: return vk::Format::eR8Sint;
			case EFormat::eR8Srgb: return vk::Format::eR8Srgb;
			case EFormat::eR8G8Unorm: return vk::Format::eR8G8Unorm;
			case EFormat::eR8G8Snorm: return vk::Format::eR8G8Snorm;
			case EFormat::eR8G8Uint: return vk::Format::eR8G8Uint;
			case EFormat::eR8G8Sint: return vk::Format::eR8G8Sint;
			case EFormat::eR8G8Srgb: return vk::Format::eR8G8Srgb;
			case EFormat::eR8G8B8Unorm: return vk::Format::eR8G8B8Unorm;
			case EFormat::eR8G8B8Snorm: return vk::Format::eR8G8B8Snorm;
			case EFormat::eR8G8B8Uint: return vk::Format::eR8G8B8Uint;
			case EFormat::eR8G8B8Sint: return vk::Format::eR8G8B8Sint;
			case EFormat::eR8G8B8Srgb: return vk::Format::eR8G8B8Srgb;
			case EFormat::eB8G8R8Unorm: return vk::Format::eB8G8R8Unorm;
			case EFormat::eB8G8R8Snorm: return vk::Format::eB8G8R8Snorm;
			case EFormat::eB8G8R8Uint: return vk::Format::eB8G8R8Uint;
			case EFormat::eB8G8R8Sint: return vk::Format::eB8G8R8Sint;
			case EFormat::eB8G8R8Srgb: return vk::Format::eB8G8R8Srgb;
			case EFormat::eR8G8B8A8Unorm: return vk::Format::eR8G8B8A8Unorm;
			case EFormat::eR8G8B8A8Snorm: return vk::Format::eR8G8B8A8Snorm;
			case EFormat::eR8G8B8A8Uint: return vk::Format::eR8G8B8A8Uint;
			case EFormat::eR8G8B8A8Sint: return vk::Format::eR8G8B8A8Sint;
			case EFormat::eR8G8B8A8Srgb: return vk::Format::eR8G8B8A8Srgb;
			case EFormat::eB8G8R8A8Unorm: return vk::Format::eB8G8R8A8Unorm;
			case EFormat::eB8G8R8A8Snorm: return vk::Format::eB8G8R8A8Snorm;
			case EFormat::eB8G8R8A8Uint: return vk::Format::eB8G8R8A8Uint;
			case EFormat::eB8G8R8A8Sint: return vk::Format::eB8G8R8A8Sint;
			case EFormat::eB8G8R8A8Srgb: return vk::Format::eB8G8R8A8Srgb;
			case EFormat::eR16Unorm: return vk::Format::eR16Unorm;
			case EFormat::eR16Snorm: return vk::Format::eR16Snorm;
			case EFormat::eR16Uint: return vk::Format::eR16Uint;
			case EFormat::eR16Sint: return vk::Format::eR16Sint;
			case EFormat::eR16Sfloat: return vk::Format::eR16Sfloat;
			case EFormat::eR16G16Unorm: return vk::Format::eR16G16Unorm;
			case EFormat::eR16G16Snorm: return vk::Format::eR16G16Snorm;
			case EFormat::eR16G16Uint: return vk::Format::eR16G16Uint;
			case EFormat::eR16G16Sint: return vk::Format::eR16G16Sint;
			case EFormat::eR16G16Sfloat: return vk::Format::eR16G16Sfloat;
			case EFormat::eR16G16B16Unorm: return vk::Format::eR16G16B16Unorm;
			case EFormat::eR16G16B16Snorm: return vk::Format::eR16G16B16Snorm;
			case EFormat::eR16G16B16Uint: return vk::Format::eR16G16B16Uint;
			case EFormat::eR16G16B16Sint: return vk::Format::eR16G16B16Sint;
			case EFormat::eR16G16B16Sfloat: return vk::Format::eR16G16B16Sfloat;
			case EFormat::eR16G16B16A16Unorm: return vk::Format::eR16G16B16A16Unorm;
			case EFormat::eR16G16B16A16Snorm: return vk::Format::eR16G16B16A16Snorm;
			case EFormat::eR16G16B16A16Uint: return vk::Format::eR16G16B16A16Uint;
			case EFormat::eR16G16B16A16Sint: return vk::Format::eR16G16B16A16Sint;
			case EFormat::eR16G16B16A16Sfloat: return vk::Format::eR16G16B16A16Sfloat;
			case EFormat::eR32Uint: return vk::Format::eR32Uint;
			case EFormat::eR32Sint: return vk::Format::eR32Sint;
			case EFormat::eR32Sfloat: return vk::Format::eR32Sfloat;
			case EFormat::eR32G32Uint: return vk::Format::eR32G32Uint;
			case EFormat::eR32G32Sint: return vk::Format::eR32G32Sint;
			case EFormat::eR32G32Sfloat: return vk::Format::eR32G32Sfloat;
			case EFormat::eR32G32B32Uint: return vk::Format::eR32G32B32Uint;
			case EFormat::eR32G32B32Sint: return vk::Format::eR32G32B32Sint;
			case EFormat::eR32G32B32Sfloat: return vk::Format::eR32G32B32Sfloat;
			case EFormat::eR32G32B32A32Uint: return vk::Format::eR32G32B32A32Uint;
			case EFormat::eR32G32B32A32Sint: return vk::Format::eR32G32B32A32Sint;
			case EFormat::eR32G32B32A32Sfloat: return vk::Format::eR32G32B32A32Sfloat;
			case EFormat::eR64Uint: return vk::Format::eR64Uint;
			case EFormat::eR64Sint: return vk::Format::eR64Sint;
			case EFormat::eR64Sfloat: return vk::Format::eR64Sfloat;
			case EFormat::eR64G64Uint: return vk::Format::eR64G64Uint;
			case EFormat::eR64G64Sint: return vk::Format::eR64G64Sint;
			case EFormat::eR64G64Sfloat: return vk::Format::eR64G64Sfloat;
			case EFormat::eR64G64B64Uint: return vk::Format::eR64G64B64Uint;
			case EFormat::eR64G64B64Sint: return vk::Format::eR64G64B64Sint;
			case EFormat::eR64G64B64Sfloat: return vk::Format::eR64G64B64Sfloat;
			case EFormat::eR64G64B64A64Uint: return vk::Format::eR64G64B64A64Uint;
			case EFormat::eR64G64B64A64Sint: return vk::Format::eR64G64B64A64Sint;
			case EFormat::eR64G64B64A64Sfloat: return vk::Format::eR64G64B64A64Sfloat;
			case EFormat::eD16Unorm: return vk::Format::eD16Unorm;
			case EFormat::eD32Sfloat: return vk::Format::eD32Sfloat;
			case EFormat::eS8Uint: return vk::Format::eS8Uint;
			case EFormat::eD16UnormS8Uint: return vk::Format::eD16UnormS8Uint;
			case EFormat::eD24UnormS8Uint: return vk::Format::eD24UnormS8Uint;
			case EFormat::eD32SfloatS8Uint: return vk::Format::eD32SfloatS8Uint;
		}
		return vk::Format::eUndefined;
	}

	constexpr auto getTstFormat(vk::Format p_format) -> EFormat
	{
		switch (p_format)
		{
			case vk::Format::eUndefined: return EFormat::eUndefined;
			case vk::Format::eR8Unorm: return EFormat::eR8Unorm;
			case vk::Format::eR8Snorm: return EFormat::eR8Snorm;
			case vk::Format::eR8Uint: return EFormat::eR8Uint;
			case vk::Format::eR8Sint: return EFormat::eR8Sint;
			case vk::Format::eR8Srgb: return EFormat::eR8Srgb;
			case vk::Format::eR8G8Unorm: return EFormat::eR8G8Unorm;
			case vk::Format::eR8G8Snorm: return EFormat::eR8G8Snorm;
			case vk::Format::eR8G8Uint: return EFormat::eR8G8Uint;
			case vk::Format::eR8G8Sint: return EFormat::eR8G8Sint;
			case vk::Format::eR8G8Srgb: return EFormat::eR8G8Srgb;
			case vk::Format::eR8G8B8Unorm: return EFormat::eR8G8B8Unorm;
			case vk::Format::eR8G8B8Snorm: return EFormat::eR8G8B8Snorm;
			case vk::Format::eR8G8B8Uint: return EFormat::eR8G8B8Uint;
			case vk::Format::eR8G8B8Sint: return EFormat::eR8G8B8Sint;
			case vk::Format::eR8G8B8Srgb: return EFormat::eR8G8B8Srgb;
			case vk::Format::eB8G8R8Unorm: return EFormat::eB8G8R8Unorm;
			case vk::Format::eB8G8R8Snorm: return EFormat::eB8G8R8Snorm;
			case vk::Format::eB8G8R8Uint: return EFormat::eB8G8R8Uint;
			case vk::Format::eB8G8R8Sint: return EFormat::eB8G8R8Sint;
			case vk::Format::eB8G8R8Srgb: return EFormat::eB8G8R8Srgb;
			case vk::Format::eR8G8B8A8Unorm: return EFormat::eR8G8B8A8Unorm;
			case vk::Format::eR8G8B8A8Snorm: return EFormat::eR8G8B8A8Snorm;
			case vk::Format::eR8G8B8A8Uint: return EFormat::eR8G8B8A8Uint;
			case vk::Format::eR8G8B8A8Sint: return EFormat::eR8G8B8A8Sint;
			case vk::Format::eR8G8B8A8Srgb: return EFormat::eR8G8B8A8Srgb;
			case vk::Format::eB8G8R8A8Unorm: return EFormat::eB8G8R8A8Unorm;
			case vk::Format::eB8G8R8A8Snorm: return EFormat::eB8G8R8A8Snorm;
			case vk::Format::eB8G8R8A8Uint: return EFormat::eB8G8R8A8Uint;
			case vk::Format::eB8G8R8A8Sint: return EFormat::eB8G8R8A8Sint;
			case vk::Format::eB8G8R8A8Srgb: return EFormat::eB8G8R8A8Srgb;
			case vk::Format::eR16Unorm: return EFormat::eR16Unorm;
			case vk::Format::eR16Snorm: return EFormat::eR16Snorm;
			case vk::Format::eR16Uint: return EFormat::eR16Uint;
			case vk::Format::eR16Sint: return EFormat::eR16Sint;
			case vk::Format::eR16Sfloat: return EFormat::eR16Sfloat;
			case vk::Format::eR16G16Unorm: return EFormat::eR16G16Unorm;
			case vk::Format::eR16G16Snorm: return EFormat::eR16G16Snorm;
			case vk::Format::eR16G16Uint: return EFormat::eR16G16Uint;
			case vk::Format::eR16G16Sint: return EFormat::eR16G16Sint;
			case vk::Format::eR16G16Sfloat: return EFormat::eR16G16Sfloat;
			case vk::Format::eR16G16B16Unorm: return EFormat::eR16G16B16Unorm;
			case vk::Format::eR16G16B16Snorm: return EFormat::eR16G16B16Snorm;
			case vk::Format::eR16G16B16Uint: return EFormat::eR16G16B16Uint;
			case vk::Format::eR16G16B16Sint: return EFormat::eR16G16B16Sint;
			case vk::Format::eR16G16B16Sfloat: return EFormat::eR16G16B16Sfloat;
			case vk::Format::eR16G16B16A16Unorm: return EFormat::eR16G16B16A16Unorm;
			case vk::Format::eR16G16B16A16Snorm: return EFormat::eR16G16B16A16Snorm;
			case vk::Format::eR16G16B16A16Uint: return EFormat::eR16G16B16A16Uint;
			case vk::Format::eR16G16B16A16Sint: return EFormat::eR16G16B16A16Sint;
			case vk::Format::eR16G16B16A16Sfloat: return EFormat::eR16G16B16A16Sfloat;
			case vk::Format::eR32Uint: return EFormat::eR32Uint;
			case vk::Format::eR32Sint: return EFormat::eR32Sint;
			case vk::Format::eR32Sfloat: return EFormat::eR32Sfloat;
			case vk::Format::eR32G32Uint: return EFormat::eR32G32Uint;
			case vk::Format::eR32G32Sint: return EFormat::eR32G32Sint;
			case vk::Format::eR32G32Sfloat: return EFormat::eR32G32Sfloat;
			case vk::Format::eR32G32B32Uint: return EFormat::eR32G32B32Uint;
			case vk::Format::eR32G32B32Sint: return EFormat::eR32G32B32Sint;
			case vk::Format::eR32G32B32Sfloat: return EFormat::eR32G32B32Sfloat;
			case vk::Format::eR32G32B32A32Uint: return EFormat::eR32G32B32A32Uint;
			case vk::Format::eR32G32B32A32Sint: return EFormat::eR32G32B32A32Sint;
			case vk::Format::eR32G32B32A32Sfloat: return EFormat::eR32G32B32A32Sfloat;
			case vk::Format::eR64Uint: return EFormat::eR64Uint;
			case vk::Format::eR64Sint: return EFormat::eR64Sint;
			case vk::Format::eR64Sfloat: return EFormat::eR64Sfloat;
			case vk::Format::eR64G64Uint: return EFormat::eR64G64Uint;
			case vk::Format::eR64G64Sint: return EFormat::eR64G64Sint;
			case vk::Format::eR64G64Sfloat: return EFormat::eR64G64Sfloat;
			case vk::Format::eR64G64B64Uint: return EFormat::eR64G64B64Uint;
			case vk::Format::eR64G64B64Sint: return EFormat::eR64G64B64Sint;
			case vk::Format::eR64G64B64Sfloat: return EFormat::eR64G64B64Sfloat;
			case vk::Format::eR64G64B64A64Uint: return EFormat::eR64G64B64A64Uint;
			case vk::Format::eR64G64B64A64Sint: return EFormat::eR64G64B64A64Sint;
			case vk::Format::eR64G64B64A64Sfloat: return EFormat::eR64G64B64A64Sfloat;
			case vk::Format::eD16Unorm: return EFormat::eD16Unorm;
			case vk::Format::eD32Sfloat: return EFormat::eD32Sfloat;
			case vk::Format::eS8Uint: return EFormat::eS8Uint;
			case vk::Format::eD16UnormS8Uint: return EFormat::eD16UnormS8Uint;
			case vk::Format::eD24UnormS8Uint: return EFormat::eD24UnormS8Uint;
			case vk::Format::eD32SfloatS8Uint: return EFormat::eD32SfloatS8Uint;
		}
		return EFormat::eUndefined;
	}

	constexpr auto hasStencilComponent(EFormat p_format) -> bool
	{
		return p_format == EFormat::eD32SfloatS8Uint || p_format == EFormat::eD24UnormS8Uint;
	}

	constexpr auto isDepthFormat(EFormat p_format) -> bool
	{
		return p_format == EFormat::eD16Unorm || p_format == EFormat::eD16UnormS8Uint || p_format == EFormat::eD24UnormS8Uint || p_format == EFormat::eD32Sfloat ||
			   p_format == EFormat::eD32SfloatS8Uint;
	}

	constexpr auto getImageAspectMask(EFormat p_format) -> vk::ImageAspectFlags
	{
		vk::ImageAspectFlags aspect_mask{isDepthFormat(p_format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor};
		aspect_mask |= hasStencilComponent(p_format) ? vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits::eNone;
		return aspect_mask;
	}

	auto getLoadOp(EAttachmentUsageOP p_usage_op) -> vk::AttachmentLoadOp
	{
		switch (p_usage_op)
		{
			case EAttachmentUsageOP::eClearStore:
			case EAttachmentUsageOP::eClearNone:
			case EAttachmentUsageOP::eClearDontCare:
				return vk::AttachmentLoadOp::eClear;
			case EAttachmentUsageOP::eLoadStore:
			case EAttachmentUsageOP::eLoadNone:
			case EAttachmentUsageOP::eLoadDontCare:
				return vk::AttachmentLoadOp::eLoad;
			case EAttachmentUsageOP::eNoneStore:
			case EAttachmentUsageOP::eNoneNone:
			case EAttachmentUsageOP::eNoneDontCare:
				return vk::AttachmentLoadOp::eNone;
			case EAttachmentUsageOP::eDontCareStore:
			case EAttachmentUsageOP::eDontCareNone:
			case EAttachmentUsageOP::eDontCareDontCare:
				return vk::AttachmentLoadOp::eDontCare;
		}
		return vk::AttachmentLoadOp::eNone;
	}

	auto getStoreOp(EAttachmentUsageOP p_usage_op) -> vk::AttachmentStoreOp
	{
		switch (p_usage_op)
		{
			case EAttachmentUsageOP::eClearStore:
			case EAttachmentUsageOP::eLoadStore:
			case EAttachmentUsageOP::eNoneStore:
			case EAttachmentUsageOP::eDontCareStore:
				return vk::AttachmentStoreOp::eStore;
			case EAttachmentUsageOP::eClearNone:
			case EAttachmentUsageOP::eLoadNone:
			case EAttachmentUsageOP::eNoneNone:
			case EAttachmentUsageOP::eDontCareNone:
				return vk::AttachmentStoreOp::eNone;
			case EAttachmentUsageOP::eClearDontCare:
			case EAttachmentUsageOP::eLoadDontCare:
			case EAttachmentUsageOP::eNoneDontCare:
			case EAttachmentUsageOP::eDontCareDontCare:
				return vk::AttachmentStoreOp::eDontCare;
		}
		return vk::AttachmentStoreOp::eNone;
	}

	auto getResolveMode(EAttachmentResolveMode p_resolve_mode) -> vk::ResolveModeFlagBits
	{
		switch (p_resolve_mode)
		{
			case EAttachmentResolveMode::eNone: return vk::ResolveModeFlagBits::eNone;
			case EAttachmentResolveMode::eZero: return vk::ResolveModeFlagBits::eSampleZero;
			case EAttachmentResolveMode::eAverage: return vk::ResolveModeFlagBits::eAverage;
			case EAttachmentResolveMode::eMin: return vk::ResolveModeFlagBits::eMin;
			case EAttachmentResolveMode::eMax: return vk::ResolveModeFlagBits::eMax;
		}
		return vk::ResolveModeFlagBits::eNone;
	}

	constexpr auto getVulkanShaderStages(EShaderStageFlags p_stages) -> vk::ShaderStageFlags
	{
		vk::ShaderStageFlags out_flags{0u};

		out_flags |= (p_stages & EShaderStageFlagBits::eVertex) ? vk::ShaderStageFlagBits::eVertex : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::ePixel) ? vk::ShaderStageFlagBits::eFragment : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::eCompute) ? vk::ShaderStageFlagBits::eCompute : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::eGeometry) ? vk::ShaderStageFlagBits::eGeometry : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::eTessControl) ? vk::ShaderStageFlagBits::eTessellationControl : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::eTessEval) ? vk::ShaderStageFlagBits::eTessellationEvaluation : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::eTask) ? vk::ShaderStageFlagBits::eTaskEXT : vk::ShaderStageFlagBits{0u};
		out_flags |= (p_stages & EShaderStageFlagBits::eMesh) ? vk::ShaderStageFlagBits::eMeshEXT : vk::ShaderStageFlagBits{0u};

		return out_flags;
	}

	constexpr auto getVulkanSampleCount(ESampleCount p_sample_count) -> vk::SampleCountFlagBits
	{
		switch (p_sample_count)
		{
			case ESampleCount::e1: return vk::SampleCountFlagBits::e1;
				break;
			case ESampleCount::e2: return vk::SampleCountFlagBits::e2;
				break;
			case ESampleCount::e4: return vk::SampleCountFlagBits::e4;
				break;
			case ESampleCount::e8: return vk::SampleCountFlagBits::e8;
				break;
			case ESampleCount::e16: return vk::SampleCountFlagBits::e16;
				break;
			case ESampleCount::e32: return vk::SampleCountFlagBits::e32;
				break;
			case ESampleCount::e64: return vk::SampleCountFlagBits::e64;
				break;
		}
		return vk::SampleCountFlagBits::e1;
	}

	constexpr auto getVulkanAccessFlags(EAccessFlags p_access_flags) -> vk::AccessFlags2
	{
		vk::AccessFlags2 out_flags{0u};

		out_flags |= (p_access_flags & EAccessFlagBits::eNone) ? vk::AccessFlagBits2::eNone : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eIndirectCommandRead) ? vk::AccessFlagBits2::eIndirectCommandRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eIndexRead) ? vk::AccessFlagBits2::eIndexRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eVertexAttributeRead) ? vk::AccessFlagBits2::eVertexAttributeRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eUniformRead) ? vk::AccessFlagBits2::eUniformRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eInputAttachmentRead) ? vk::AccessFlagBits2::eInputAttachmentRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eShaderRead) ? vk::AccessFlagBits2::eShaderRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eShaderWrite) ? vk::AccessFlagBits2::eShaderWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eColorAttachmentRead) ? vk::AccessFlagBits2::eColorAttachmentRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eColorAttachmentWrite) ? vk::AccessFlagBits2::eColorAttachmentWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eDepthStencilAttachmentRead) ? vk::AccessFlagBits2::eDepthStencilAttachmentRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eDepthStencilAttachmentWrite) ? vk::AccessFlagBits2::eDepthStencilAttachmentWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eTransferRead) ? vk::AccessFlagBits2::eTransferRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eTransferWrite) ? vk::AccessFlagBits2::eTransferWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eHostRead) ? vk::AccessFlagBits2::eHostRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eHostWrite) ? vk::AccessFlagBits2::eHostWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eMemoryRead) ? vk::AccessFlagBits2::eMemoryRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eMemoryWrite) ? vk::AccessFlagBits2::eMemoryWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eShaderSampledRead) ? vk::AccessFlagBits2::eShaderSampledRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eShaderStorageRead) ? vk::AccessFlagBits2::eShaderStorageRead : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eShaderStorageWrite) ? vk::AccessFlagBits2::eShaderStorageWrite : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eSamplerHeapRead) ? vk::AccessFlagBits2::eSamplerHeapReadEXT : vk::AccessFlagBits2{0u};
		out_flags |= (p_access_flags & EAccessFlagBits::eResourceHeapRead) ? vk::AccessFlagBits2::eResourceHeapReadEXT : vk::AccessFlagBits2{0u};

		return out_flags;
	}

	constexpr auto getVulkanPipelineStageFlags(EPipelineStageFlags p_pipeline_stage_flags) -> vk::PipelineStageFlags2
	{
		vk::PipelineStageFlags2 out_flags{0u};

		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eNone) ? vk::PipelineStageFlagBits2::eNone : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eTopOfPipe) ? vk::PipelineStageFlagBits2::eTopOfPipe : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eDrawIndirect) ? vk::PipelineStageFlagBits2::eDrawIndirect : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eVertexInput) ? vk::PipelineStageFlagBits2::eVertexInput : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eVertexShader) ? vk::PipelineStageFlagBits2::eVertexShader : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eTessellationControlShader)
						 ? vk::PipelineStageFlagBits2::eTessellationControlShader
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eTessellationEvaluationShader)
						 ? vk::PipelineStageFlagBits2::eTessellationEvaluationShader
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eGeometryShader) ? vk::PipelineStageFlagBits2::eGeometryShader : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eFragmentShader) ? vk::PipelineStageFlagBits2::eFragmentShader : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eEarlyFragmentTests)
						 ? vk::PipelineStageFlagBits2::eEarlyFragmentTests
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eLateFragmentTests)
						 ? vk::PipelineStageFlagBits2::eLateFragmentTests
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eColorAttachmentOutput)
						 ? vk::PipelineStageFlagBits2::eColorAttachmentOutput
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eComputeShader) ? vk::PipelineStageFlagBits2::eComputeShader : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eAllTransfer) ? vk::PipelineStageFlagBits2::eAllTransfer : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eTransfer) ? vk::PipelineStageFlagBits2::eTransfer : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eBottomOfPipe) ? vk::PipelineStageFlagBits2::eBottomOfPipe : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eHost) ? vk::PipelineStageFlagBits2::eHost : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eAllGraphics) ? vk::PipelineStageFlagBits2::eAllGraphics : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eAllCommands) ? vk::PipelineStageFlagBits2::eAllCommands : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eCopy) ? vk::PipelineStageFlagBits2::eCopy : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eResolve) ? vk::PipelineStageFlagBits2::eResolve : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eBlit) ? vk::PipelineStageFlagBits2::eBlit : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eClear) ? vk::PipelineStageFlagBits2::eClear : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eIndexInput) ? vk::PipelineStageFlagBits2::eIndexInput : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eVertexAttributeInput)
						 ? vk::PipelineStageFlagBits2::eVertexAttributeInput
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::ePreRasterizationShaders)
						 ? vk::PipelineStageFlagBits2::ePreRasterizationShaders
						 : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eTaskShader) ? vk::PipelineStageFlagBits2::eTaskShaderEXT : vk::PipelineStageFlagBits2{0u};
		out_flags |= (p_pipeline_stage_flags & EPipelineStageFlagBits::eMeshShader) ? vk::PipelineStageFlagBits2::eMeshShaderEXT : vk::PipelineStageFlagBits2{0u};

		return out_flags;
	}

	auto gpu_ctx_create(const GPUContextDesc &p_desc) -> GPUContext *
	{
		GPUContext *ctx{new GPUContext{}};

		ctx->usingSwapchain                  = p_desc.usingSwapchain;
		ctx->maxConcurrentSwapchainWorkloads = p_desc.maxConcurrentSwapchainWorkloads;

		for (uint32 i{0u}; i < 3u; ++i)
			mtx_init(&ctx->queueMutexes[i], mtx_plain);
		mtx_init(&ctx->freeCommandListMutex, mtx_plain);
		mtx_init(&ctx->undefinedTexturesMutex, mtx_plain);

		#pragma region instance

		FunctionDispatcher::initBaseFunctions();

		ExtensionSet required_extensions;

		if (p_desc.usingSwapchain)
		{
			required_extensions.emplace(vk::KHRSurfaceExtensionName);
			required_extensions.emplace(vk::KHRWin32SurfaceExtensionName);
		}
		if (p_desc.enableDebugInfo)
			required_extensions.emplace(vk::EXTDebugUtilsExtensionName);

		{
			auto required_extensions_vec{required_extensions | std::ranges::to<std::vector>()};

			std::vector extension_props{vk::enumerateInstanceExtensionProperties()};

			// Make sure that all the required extensions are present in the extension_props vector
			const auto unsupported_extension{
				std::ranges::find_if(required_extensions_vec, [extension_props](const auto &extension)
				{
					// returns true if none of the extensions are present (the strcmp would always evaluate to false)
					return std::ranges::none_of(extension_props, [ext = extension](const auto &prop)
					{
						return std::strcmp(prop.extensionName.data(), ext) == 0;
					});
				})
			};

			if (unsupported_extension != required_extensions_vec.end())
			{
				// We can't continue without the required extensions, so terminate the program here
				std::printf("Required extension \"%s\" is not supported", *unsupported_extension);
				TST_PERMA_ASSERT(false);
			}

			std::vector<CString> required_validation_layers;
			if (p_desc.enableDebugInfo)
			{
				required_validation_layers.emplace_back("VK_LAYER_KHRONOS_validation");
			}

			std::vector layer_props{vk::enumerateInstanceLayerProperties()};

			// Finds any layer in required_validation_layers, such that it is also not present in the actual layer_props vector
			const auto unsupported_layer_it{
				std::ranges::find_if(required_validation_layers, [layer_props](const auto &layer)
				{
					return std::ranges::none_of(layer_props, [layer](const auto &prop)
					{
						return std::strcmp(prop.layerName.data(), layer) == 0;
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

			vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
			if (p_desc.enableDebugInfo)
			{
				constexpr vk::DebugUtilsMessageSeverityFlagsEXT severity_flags{
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
				};
				constexpr vk::DebugUtilsMessageTypeFlagsEXT message_type_flags{
					vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
				};
				debug_messenger_create_info.messageSeverity = severity_flags;
				debug_messenger_create_info.messageType     = message_type_flags;
				debug_messenger_create_info.pfnUserCallback = &defaultDebugCallback;
			}

			vk::ApplicationInfo application_info{};
			application_info.pApplicationName   = "Toaster";
			application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			application_info.pEngineName        = "Toaster";
			application_info.engineVersion      = VK_MAKE_VERSION(2, 1, 0);
			application_info.apiVersion         = vk::ApiVersion14;

			vk::InstanceCreateInfo instance_create_info{};
			instance_create_info.pApplicationInfo        = &application_info;
			instance_create_info.enabledExtensionCount   = required_extensions_vec.size();
			instance_create_info.ppEnabledExtensionNames = required_extensions_vec.data();

			if (p_desc.enableDebugInfo)
			{
				instance_create_info.enabledLayerCount   = required_validation_layers.size();
				instance_create_info.ppEnabledLayerNames = required_validation_layers.data();
				instance_create_info.pNext               = &debug_messenger_create_info;
			}

			ctx->vulkanInstance = vk::createInstance(instance_create_info);

			FunctionDispatcher::initInstanceFunctions(ctx->vulkanInstance);
		}

		#pragma endregion

		ExtensionSet required_device_extensions{
			vk::EXTDescriptorHeapExtensionName,
			vk::EXTShaderObjectExtensionName,
			vk::EXTColorWriteEnableExtensionName,
			vk::EXTMeshShaderExtensionName,
			vk::KHRMaintenance1ExtensionName,
			vk::KHRShaderUntypedPointersExtensionName,
			vk::KHRMaintenance9ExtensionName,
			vk::KHRUnifiedImageLayoutsExtensionName,
			vk::EXTFragmentShaderInterlockExtensionName
		};
		if (p_desc.usingSwapchain)
			required_device_extensions.emplace(vk::KHRSwapchainExtensionName);

		#pragma region physical device
		{
			auto physical_devices{ctx->vulkanInstance.enumeratePhysicalDevices()};
			if (physical_devices.empty())
			{
				// If your gpu does not have Vulkan support, we can't use Vulkan
				TST_PERMA_ASSERT_MSG(false, "Failed to find physical devices with Vulkan support");
			}

			DeviceSuitabilityFn device_suitability_check_fn{
				+[](vk::PhysicalDevice p_physical_device) -> bool
				{
					auto features{
						p_physical_device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features,
							vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT, vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR,
							vk::PhysicalDeviceDescriptorHeapFeaturesEXT, vk::PhysicalDeviceMeshShaderFeaturesEXT>()
					};

					const bool supports_required_features{
						features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy && features.get<vk::PhysicalDeviceFeatures2>().features.sampleRateShading
						&& features.get<vk::PhysicalDeviceFeatures2>().features.fillModeNonSolid && features.get<vk::PhysicalDeviceVulkan12Features>().timelineSemaphore
						&& features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
						features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState && features.get<
							vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR>().unifiedImageLayouts && features.get<vk::PhysicalDeviceDescriptorHeapFeaturesEXT>().
						descriptorHeap && features.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>().meshShader
					};

					return supports_required_features;
				}
			};

			const auto device_it{
				std::ranges::find_if(physical_devices, [&](const auto &device)
				{
					return isDeviceSuitable(device, required_device_extensions, device_suitability_check_fn);
				})
			};

			if (device_it == physical_devices.end())
			{
				TST_PERMA_ASSERT_MSG(false, "Failed to find suitable physical device");
			}

			ctx->physicalDevice = *device_it;

			ctx->deviceProperties.pNext = &ctx->descriptorHeapProperties;
			ctx->physicalDevice.getProperties2(&ctx->deviceProperties);
		}

		#pragma endregion

		#pragma region logical device

		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceVulkan14Features, vk::PhysicalDeviceShaderObjectFeaturesEXT, vk::PhysicalDeviceDescriptorHeapFeaturesEXT,
			vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR, vk::PhysicalDeviceMaintenance9FeaturesKHR, vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR,
			vk::PhysicalDeviceMeshShaderFeaturesEXT, vk::PhysicalDeviceColorWriteEnableFeaturesEXT, vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT> feature_chain{
			{},
			{},
			{},
			{},
			{},
			{},
			{},
			{},
			{},
			{},
			{},
			{},
			{}
		};

		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy                             = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.sampleRateShading                             = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.fillModeNonSolid                              = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.fragmentStoresAndAtomics                      = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.shaderInt16                                   = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.shaderInt64                                   = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.vertexPipelineStoresAndAtomics                = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect                             = true;
		feature_chain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters                            = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().scalarBlockLayout                               = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().timelineSemaphore                               = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress                             = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().runtimeDescriptorArray                          = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderInt8                                      = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().storagePushConstant8                            = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().uniformAndStorageBuffer8BitAccess               = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().storageBuffer8BitAccess                         = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderStorageBufferArrayNonUniformIndexing      = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderUniformBufferArrayNonUniformIndexing      = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderSampledImageArrayNonUniformIndexing       = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderUniformBufferArrayNonUniformIndexing      = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderStorageImageArrayNonUniformIndexing       = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().drawIndirectCount                               = true;
		feature_chain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering                                = true;
		feature_chain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2                                = true;
		feature_chain.get<vk::PhysicalDeviceVulkan13Features>().maintenance4                                    = true;
		feature_chain.get<vk::PhysicalDeviceVulkan14Features>().indexTypeUint8                                  = true;
		feature_chain.get<vk::PhysicalDeviceVulkan14Features>().maintenance6                                    = true;
		feature_chain.get<vk::PhysicalDeviceShaderObjectFeaturesEXT>().shaderObject                             = true;
		feature_chain.get<vk::PhysicalDeviceDescriptorHeapFeaturesEXT>().descriptorHeap                         = true;
		feature_chain.get<vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR>().shaderUntypedPointers           = true;
		feature_chain.get<vk::PhysicalDeviceMaintenance9FeaturesKHR>().maintenance9                             = true;
		feature_chain.get<vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR>().unifiedImageLayouts               = true;
		feature_chain.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>().meshShader                                 = true;
		feature_chain.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>().taskShader                                 = true;
		feature_chain.get<vk::PhysicalDeviceColorWriteEnableFeaturesEXT>().colorWriteEnable                     = true;
		feature_chain.get<vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT>().fragmentShaderPixelInterlock  = true;
		feature_chain.get<vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT>().fragmentShaderSampleInterlock = true;

		ctx->queueFamilyIndices = selectQueueFamilyIndices(ctx->physicalDevice, p_desc.usingSwapchain);

		if (ctx->queueFamilyIndices.graphics == UINT32_MAX)
			TST_PERMA_ASSERT_MSG(false, "Failed to find a graphics queue family");

		if (ctx->queueFamilyIndices.transfer == UINT32_MAX)
			ctx->queueFamilyIndices.transfer = ctx->queueFamilyIndices.graphics;

		constexpr float32 default_queue_priority{1.0f};

		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		queue_create_infos.emplace_back(vk::DeviceQueueCreateInfo{{}, ctx->queueFamilyIndices.graphics, 1u, &default_queue_priority});
		if (ctx->queueFamilyIndices.transfer != ctx->queueFamilyIndices.graphics)
			queue_create_infos.emplace_back(vk::DeviceQueueCreateInfo{{}, ctx->queueFamilyIndices.transfer, 1u, &default_queue_priority});

		const auto enabled_extensions_vec{required_device_extensions | std::ranges::to<std::vector>()};

		vk::DeviceCreateInfo logical_device_create_info{};
		logical_device_create_info.queueCreateInfoCount    = queue_create_infos.size();
		logical_device_create_info.pQueueCreateInfos       = queue_create_infos.data();
		logical_device_create_info.enabledExtensionCount   = enabled_extensions_vec.size();
		logical_device_create_info.ppEnabledExtensionNames = enabled_extensions_vec.data();
		logical_device_create_info.pNext                   = feature_chain.get<vk::PhysicalDeviceFeatures2>();

		ctx->logicalDevice = ctx->physicalDevice.createDevice(logical_device_create_info);

		vk::Queue graphics_queue{ctx->logicalDevice.getQueue(ctx->queueFamilyIndices.graphics, 0u)};
		vk::Queue compute_queue{graphics_queue}; // TODO: More queues
		vk::Queue transfer_queue{
			ctx->logicalDevice.getQueue(ctx->queueFamilyIndices.transfer, (ctx->queueFamilyIndices.transfer == ctx->queueFamilyIndices.graphics) ? 1u : 0u)
		};

		ctx->queues[0] = graphics_queue;
		ctx->queues[1] = compute_queue;
		ctx->queues[2] = transfer_queue;

		for (uint32 queue_index{0u}; queue_index < 3u; ++queue_index)
		{
			vk::CommandPoolCreateInfo command_pool_create_info{};
			command_pool_create_info.queueFamilyIndex = ctx->queueFamilyIndices.get(static_cast<EQueueType>(queue_index));
			command_pool_create_info.flags            = vk::CommandPoolCreateFlagBits::eTransient;
			ctx->transientPools[queue_index]          = ctx->logicalDevice.createCommandPool(command_pool_create_info);
		}

		FunctionDispatcher::initDeviceFunctions(ctx->logicalDevice);

		#pragma endregion

		#pragma region allocator

		VmaAllocatorCreateInfo allocator_create_info{};
		allocator_create_info.instance       = ctx->vulkanInstance;
		allocator_create_info.physicalDevice = ctx->physicalDevice;
		allocator_create_info.device         = ctx->logicalDevice;
		allocator_create_info.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocator_create_info, &ctx->allocator);

		#pragma endregion

		#pragma region destroy callbacks

		ctx->commandLists.setDestructorUserData(ctx);
		ctx->commandLists.setDestructorFn(+[](CommandList *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			ctx->logicalDevice.destroyCommandPool(p_data->pool);
		});

		ctx->resourceHeaps.setDestructorUserData(ctx);
		ctx->resourceHeaps.setDestructorFn(+[](ResourceDescriptorHeap *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			vmaUnmapMemory(ctx->allocator, p_data->heapAllocation);
			vmaDestroyBuffer(ctx->allocator, p_data->heapBuffer, p_data->heapAllocation);
			mtx_destroy(&p_data->imageSlotMutex);
			mtx_destroy(&p_data->bufferSlotMutex);
		});

		ctx->samplerHeaps.setDestructorUserData(ctx);
		ctx->samplerHeaps.setDestructorFn(+[](SamplerDescriptorHeap *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			vmaUnmapMemory(ctx->allocator, p_data->heapAllocation);
			vmaDestroyBuffer(ctx->allocator, p_data->heapBuffer, p_data->heapAllocation);
			mtx_destroy(&p_data->samplerSlotMutex);
		});

		ctx->semaphores.setDestructorUserData(ctx);
		ctx->semaphores.setDestructorFn(+[](Semaphore *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			ctx->logicalDevice.destroySemaphore(p_data->semaphore);
		});

		ctx->buffers.setDestructorUserData(ctx);
		ctx->buffers.setDestructorFn(+[](Buffer *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			vmaDestroyBuffer(ctx->allocator, p_data->buffer, p_data->allocation);
			p_data->buffer     = nullptr;
			p_data->allocation = nullptr;
			p_data->mapped     = nullptr;
		});

		ctx->textures.setDestructorUserData(ctx);
		ctx->textures.setDestructorFn(+[](Texture *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			if (p_data->imageView)
				ctx->logicalDevice.destroyImageView(p_data->imageView);

			if (!p_data->isSwapchainImage)
				vmaDestroyImage(ctx->allocator, p_data->image, p_data->allocation);

			p_data->image      = nullptr;
			p_data->allocation = nullptr;
			p_data->imageView  = nullptr;
		});

		ctx->surfaces.setDestructorUserData(ctx);
		ctx->surfaces.setDestructorFn(+[](Surface *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			ctx->vulkanInstance.destroySurfaceKHR(p_data->surface);
		});

		ctx->swapchains.setDestructorUserData(ctx);
		ctx->swapchains.setDestructorFn(+[](Swapchain *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			for (auto &attachment: p_data->attachments)
				ctx->textures.destroy(attachment);

			for (auto &semaphore: p_data->imageAvailableSemaphores)
				ctx->logicalDevice.destroySemaphore(semaphore);

			for (auto &semaphore: p_data->renderFinishedSemaphores)
				ctx->logicalDevice.destroySemaphore(semaphore);

			ctx->logicalDevice.destroySwapchainKHR(p_data->swapchain);
		});

		ctx->shaders.setDestructorUserData(ctx);
		ctx->shaders.setDestructorFn(+[](Shader *p_data, void *p_ctx) mutable noexcept-> void
		{
			GPUContext *ctx{static_cast<GPUContext *>(p_ctx)};
			ctx->logicalDevice.destroyShaderEXT(p_data->shader, nullptr, FunctionDispatcher::get());
		});

		#pragma endregion

		return ctx;
	}

	auto gpu_ctx_destroy(GPUContext *p_ctx) -> void
	{
		p_ctx->logicalDevice.waitIdle();

		p_ctx->resourceHeaps.clear();
		p_ctx->samplerHeaps.clear();
		p_ctx->textures.clear();
		p_ctx->samplers.clear();
		p_ctx->buffers.clear();
		p_ctx->shaders.clear();
		p_ctx->surfaces.clear();
		p_ctx->semaphores.clear();
		p_ctx->commandLists.clear();

		for (uint32 queue_index{0u}; queue_index < 3u; ++queue_index)
			p_ctx->logicalDevice.destroyCommandPool(p_ctx->transientPools[queue_index]);

		vmaDestroyAllocator(p_ctx->allocator);
		p_ctx->logicalDevice.destroy();
		p_ctx->vulkanInstance.destroy();

		for (uint32 i{0u}; i < 3u; ++i)
			mtx_destroy(&p_ctx->queueMutexes[i]);
		mtx_destroy(&p_ctx->freeCommandListMutex);
		mtx_destroy(&p_ctx->undefinedTexturesMutex);

		delete p_ctx;
	}

	auto wait_idle(GPUContext *p_ctx) -> void
	{
		p_ctx->logicalDevice.waitIdle();
	}

	auto wait_queue_idle(GPUContext *p_ctx, EQueueType p_queue_type) -> void
	{
		vk::Queue queue{p_ctx->queues[static_cast<uint32>(p_queue_type)]};
		queue.waitIdle();
	}

	auto getPendingImageMemoryBarriers(GPUContext *p_ctx, EQueueType p_queue_type) -> vk::CommandBuffer
	{
		mtx_lock(&p_ctx->undefinedTexturesMutex);

		if (p_ctx->undefinedTextures.empty() || p_queue_type == EQueueType::eTransfer) // Transfer queues cannot perform layout transitions
		{
			mtx_unlock(&p_ctx->undefinedTexturesMutex);
			return nullptr;
		}

		std::vector<vk::ImageMemoryBarrier2> image_memory_barriers(p_ctx->undefinedTextures.size());

		uint32 i{0u};
		for (auto undef_tex: p_ctx->undefinedTextures)
		{
			const Texture &texture{p_ctx->textures[undef_tex]};
			TST_PERMA_ASSERT(p_ctx->textures.isValid(undef_tex));

			image_memory_barriers[i].srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe;
			image_memory_barriers[i].srcAccessMask       = vk::AccessFlagBits2::eNone;
			image_memory_barriers[i].dstStageMask        = vk::PipelineStageFlagBits2::eAllCommands;
			image_memory_barriers[i].dstAccessMask       = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite; // Covers every access flag basically
			image_memory_barriers[i].oldLayout           = vk::ImageLayout::eUndefined;
			image_memory_barriers[i].newLayout           = vk::ImageLayout::eGeneral;
			image_memory_barriers[i].srcQueueFamilyIndex = vk::QueueFamilyIgnored;
			image_memory_barriers[i].dstQueueFamilyIndex = vk::QueueFamilyIgnored;
			image_memory_barriers[i].image               = texture.image;
			image_memory_barriers[i].subresourceRange    = vk::ImageSubresourceRange{
				getImageAspectMask(texture.desc.format),
				0u,
				texture.desc.mipCount,
				0u,
				texture.desc.layerCount
			};
			++i;
		}

		mtx_unlock(&p_ctx->undefinedTexturesMutex);

		vk::DependencyInfo dependency_info{};
		dependency_info.setImageMemoryBarriers(image_memory_barriers);

		vk::CommandPool pool{p_ctx->transientPools[static_cast<uint32>(p_queue_type)]};

		vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info.commandPool        = pool;
		command_buffer_alloc_info.commandBufferCount = 1u;
		command_buffer_alloc_info.level              = vk::CommandBufferLevel::ePrimary;
		vk::CommandBuffer cmd{p_ctx->logicalDevice.allocateCommandBuffers(command_buffer_alloc_info).front()};

		p_ctx->logicalDevice.setDebugUtilsObjectNameEXT(cmd, "Image memory barrier cmd", FunctionDispatcher::get());

		cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		cmd.pipelineBarrier2(dependency_info);
		cmd.end();

		return cmd;
	}

	auto cmd_list_get_or_create(GPUContext *p_ctx, EQueueType p_queue_type, bool p_secondary) -> CommandListHandle
	{
		TST_ASSERT(p_ctx);
		{
			mtx_lock(&p_ctx->freeCommandListMutex);

			auto &free_lists{p_secondary ? p_ctx->freeSecondaryLists[static_cast<uint32>(p_queue_type)] : p_ctx->freeCommandLists[static_cast<uint32>(p_queue_type)]};
			if (!free_lists.empty())
			{
				CommandListHandle command_list_handle{free_lists.back()};
				free_lists.pop_back();

				mtx_unlock(&p_ctx->freeCommandListMutex);

				return command_list_handle;
			}

			mtx_unlock(&p_ctx->freeCommandListMutex);
		}
		vk::CommandPoolCreateInfo command_pool_create_info{};
		command_pool_create_info.queueFamilyIndex = p_ctx->queueFamilyIndices.get(p_queue_type);
		command_pool_create_info.flags            = vk::CommandPoolCreateFlagBits::eTransient;
		vk::CommandPool pool{p_ctx->logicalDevice.createCommandPool(command_pool_create_info)};

		vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info.commandPool        = pool;
		command_buffer_alloc_info.commandBufferCount = 1u;
		command_buffer_alloc_info.level              = p_secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
		vk::CommandBuffer cmd{p_ctx->logicalDevice.allocateCommandBuffers(command_buffer_alloc_info).front()};

		p_ctx->logicalDevice.setDebugUtilsObjectNameEXT(cmd, p_queue_type == EQueueType::eTransfer ? "Transfer cmd" : "Graphics cmd", FunctionDispatcher::get());

		return p_ctx->commandLists.emplace(CommandList{cmd, pool, p_queue_type, false, p_secondary});
	}

	auto cmd_list_open(GPUContext *p_ctx, CommandListHandle p_cmd_list, const CommandListInheritanceInfo *p_inheritance_info) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		TST_ASSERT(!cmd.open);
		cmd.open = true;

		vk::CommandBufferInheritanceDescriptorHeapInfoEXT descriptor_heap_inheritance_info{};
		if (cmd.secondary && p_inheritance_info)
		{
			ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_inheritance_info->resourceHeap]};
			SamplerDescriptorHeap & sampler_heap{p_ctx->samplerHeaps[p_inheritance_info->samplerHeap]};

			descriptor_heap_inheritance_info.pResourceHeapBindInfo = &resource_heap.bindInfo;
			descriptor_heap_inheritance_info.pSamplerHeapBindInfo  = &sampler_heap.bindInfo;
		}

		vk::CommandBufferInheritanceRenderingInfo rendering_inheritance_info{};
		std::vector<vk::Format>                   colour_attachment_formats;
		if (cmd.secondary && p_inheritance_info)
		{
			colour_attachment_formats = p_inheritance_info->colourAttachmentFormats | std::views::transform(+[](EFormat format) -> vk::Format
			{
				return getVulkanFormat(format);
			}) | std::ranges::to<std::vector>();

			rendering_inheritance_info.colorAttachmentCount    = colour_attachment_formats.size();
			rendering_inheritance_info.pColorAttachmentFormats = colour_attachment_formats.data();
			rendering_inheritance_info.depthAttachmentFormat   = getVulkanFormat(p_inheritance_info->depthAttachmentFormat);
			rendering_inheritance_info.stencilAttachmentFormat = getVulkanFormat(p_inheritance_info->stencilAttachmentFormat);
			rendering_inheritance_info.rasterizationSamples    = getVulkanSampleCount(p_inheritance_info->samples);
			rendering_inheritance_info.pNext                   = &descriptor_heap_inheritance_info;
		}

		vk::CommandBufferInheritanceInfo inheritance_info{};
		if (cmd.secondary && p_inheritance_info)
			inheritance_info.pNext = &rendering_inheritance_info;

		vk::CommandBufferBeginInfo begin_info{};
		begin_info.flags = cmd.secondary ? vk::CommandBufferUsageFlagBits::eRenderPassContinue : vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		if (cmd.secondary && p_inheritance_info)
			begin_info.pInheritanceInfo = &inheritance_info;

		cmd.cmd.begin(begin_info);
	}

	auto cmd_list_close(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		TST_ASSERT(cmd.open);
		cmd.open = false;
		cmd.cmd.end();
	}

	auto cmd_list_free(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(!cmd.open);

		mtx_lock(&p_ctx->freeCommandListMutex);

		if (cmd.secondary)
			p_ctx->freeSecondaryLists[static_cast<uint32>(cmd.queueType)].push_back(p_cmd_list);
		else
			p_ctx->freeCommandLists[static_cast<uint32>(cmd.queueType)].push_back(p_cmd_list);

		mtx_unlock(&p_ctx->freeCommandListMutex);
	}

	auto cmd_list_reset(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &command_list{p_ctx->commandLists[p_cmd_list]};

		if (command_list.open)
			command_list.open = false;

		p_ctx->logicalDevice.resetCommandPool(command_list.pool);
	}

	// Accepts raw Vulkan semaphores
	static auto submit(GPUContext *                                   p_ctx, EQueueType p_queue_type, InitialiserList<const CommandListHandle> p_cmd_lists,
					   InitialiserList<const vk::SemaphoreSubmitInfo> p_wait_semaphore_infos,
					   InitialiserList<const vk::SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void
	{
		vk::CommandBuffer image_memory_barrier_cmd{getPendingImageMemoryBarriers(p_ctx, p_queue_type)};

		std::vector<vk::CommandBufferSubmitInfo> command_buffer_submit_infos(p_cmd_lists.size());
		for (uint64 i{0u}; i < p_cmd_lists.size(); ++i)
		{
			CommandList &cmd_list{p_ctx->commandLists[p_cmd_lists[i]]};
			TST_ASSERT(!cmd_list.open);
			TST_ASSERT_MSG(!cmd_list.secondary, "Cannot submit a secondary command buffer!");

			auto &cmd_submit_info{command_buffer_submit_infos[i]};
			cmd_submit_info.commandBuffer = cmd_list.cmd;
		}

		std::vector<vk::SemaphoreSubmitInfo> signal_infos{p_signal_semaphore_infos | std::ranges::to<std::vector>()};

		if (image_memory_barrier_cmd) // I need to make sure that the image memory barriers are submitted first so they can clear properly
		{
			command_buffer_submit_infos.insert(command_buffer_submit_infos.begin(), vk::CommandBufferSubmitInfo{image_memory_barrier_cmd});
		}

		vk::SubmitInfo2 submit_info{}; // Best of Zach 2. Best of, best of Zach 2.
		submit_info.setWaitSemaphoreInfos(p_wait_semaphore_infos);
		submit_info.setSignalSemaphoreInfos(signal_infos);
		submit_info.setCommandBufferInfos(command_buffer_submit_infos);

		vk::Queue queue{p_ctx->queues[static_cast<uint32>(p_queue_type)]};

		mtx_lock(&p_ctx->queueMutexes[TST_LIB_ENUM_VAL(p_queue_type)]); // Lock for the submit
		queue.submit2(submit_info);
		mtx_unlock(&p_ctx->queueMutexes[TST_LIB_ENUM_VAL(p_queue_type)]);
	}

	auto queue_submit(GPUContext *                               p_ctx, EQueueType p_queue_type, InitialiserList<const CommandListHandle> p_cmd_lists,
					  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos, InitialiserList<const SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void
	{
		std::vector<vk::SemaphoreSubmitInfo> wait_infos(p_wait_semaphore_infos.size());
		for (uint64 i{0u}; i < p_wait_semaphore_infos.size(); ++i)
		{
			const auto &src_wait_info{p_wait_semaphore_infos[i]};

			vk::Semaphore semaphore{p_ctx->semaphores[src_wait_info.semaphore].semaphore};

			auto &dst_wait_info{wait_infos[i]};
			dst_wait_info.semaphore = semaphore;
			dst_wait_info.value     = src_wait_info.value;
		}

		std::vector<vk::SemaphoreSubmitInfo> signal_infos(p_signal_semaphore_infos.size());
		for (uint64 i{0u}; i < p_signal_semaphore_infos.size(); ++i)
		{
			const auto &src_signal_info{p_signal_semaphore_infos[i]};

			vk::Semaphore semaphore{p_ctx->semaphores[src_signal_info.semaphore].semaphore};

			auto &dst_signal_info{signal_infos[i]};
			dst_signal_info.semaphore = semaphore;
			dst_signal_info.value     = src_signal_info.value;
		}

		submit(p_ctx, p_queue_type, p_cmd_lists, wait_infos, signal_infos);
	}

	auto cmd_execute_cmd_lists(GPUContext *p_ctx, CommandListHandle p_primary_command_list, InitialiserList<const CommandListHandle> p_secondary_command_lists) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_primary_command_list]};
		TST_ASSERT(!cmd.secondary && cmd.open);

		std::vector<vk::CommandBuffer> secondary_lists(p_secondary_command_lists.size());
		for (uint32 i{0u}; i < p_secondary_command_lists.size(); ++i)
		{
			CommandList &secondary{p_ctx->commandLists[p_secondary_command_lists[i]]};
			TST_ASSERT(secondary.secondary && !secondary.open);
			secondary_lists[i] = secondary.cmd;
		}

		cmd.cmd.executeCommands(secondary_lists, FunctionDispatcher::get());
	}

	auto cmd_copy_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_src_buffer, BufferHandle p_dst_buffer, uint64 p_size, uint64 p_src_offset,
						 uint64      p_dst_offset) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		Buffer &src_buffer{p_ctx->buffers[p_src_buffer]};
		Buffer &dst_buffer{p_ctx->buffers[p_dst_buffer]};

		vk::BufferCopy2 copy_region{};
		copy_region.size      = p_size;
		copy_region.srcOffset = p_src_offset;
		copy_region.dstOffset = p_dst_offset;

		vk::CopyBufferInfo2 copy_buffer_info{};
		copy_buffer_info.setRegions(copy_region);
		copy_buffer_info.srcBuffer = src_buffer.buffer;
		copy_buffer_info.dstBuffer = dst_buffer.buffer;

		cmd.cmd.copyBuffer2(copy_buffer_info);
	}

	auto cmd_copy_buffer_to_texture(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_src_buffer, TextureHandle p_dst_texture, uint64 p_src_offset,
									uint32      p_mip_level, uint32      p_base_layer, uint32     p_layer_count, math::UInt3  p_extent, math::Int3  p_offset) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Texture *dst_texture{p_ctx->textures.tryGet(p_dst_texture)};
		TST_PERMA_ASSERT(dst_texture);

		TST_ASSERT_MSG(dst_texture->desc.usage & ETextureUsageFlagBits::eTransferDst, "Texture was not created with the usage of transfer dst");
		TST_ASSERT_MSG(p_mip_level < dst_texture->desc.mipCount, "Texture mip level is out of range");
		TST_ASSERT_MSG(p_base_layer + p_layer_count <= dst_texture->desc.layerCount, "Texture layer range is out of range");

		if (p_extent.x == 0u)
			p_extent.x = (dst_texture->desc.extent.x >> p_mip_level) ? (dst_texture->desc.extent.x >> p_mip_level) : 1u;
		if (p_extent.y == 0u)
			p_extent.y = (dst_texture->desc.extent.y >> p_mip_level) ? (dst_texture->desc.extent.y >> p_mip_level) : 1u;
		if (p_extent.z == 0u)
			p_extent.z = (dst_texture->desc.extent.z >> p_mip_level) ? (dst_texture->desc.extent.z >> p_mip_level) : 1u;

		{
			mtx_lock(&p_ctx->undefinedTexturesMutex);

			auto undefined_it{std::ranges::find(p_ctx->undefinedTextures, p_dst_texture)};
			if (undefined_it != p_ctx->undefinedTextures.end())
			{
				vk::ImageMemoryBarrier2 barrier{};
				barrier.srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe;
				barrier.dstStageMask        = vk::PipelineStageFlagBits2::eTransfer;
				barrier.srcAccessMask       = vk::AccessFlagBits2::eNone;
				barrier.dstAccessMask       = vk::AccessFlagBits2::eTransferWrite;
				barrier.oldLayout           = vk::ImageLayout::eUndefined;
				barrier.newLayout           = vk::ImageLayout::eGeneral;
				barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
				barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
				barrier.image               = dst_texture->image;
				barrier.subresourceRange    = vk::ImageSubresourceRange{
					getImageAspectMask(dst_texture->desc.format),
					0u,
					dst_texture->desc.mipCount,
					0u,
					dst_texture->desc.layerCount
				};

				vk::DependencyInfo dependency_info{};
				dependency_info.setImageMemoryBarriers(barrier);
				cmd.cmd.pipelineBarrier2(dependency_info);
				undefined_it = p_ctx->undefinedTextures.erase(undefined_it);
			}

			mtx_unlock(&p_ctx->undefinedTexturesMutex);
		}

		vk::BufferImageCopy2 copy_region{};
		copy_region.bufferOffset      = p_src_offset;
		copy_region.bufferRowLength   = 0u; // TODO: More options for advanced copy operations
		copy_region.bufferImageHeight = 0u;
		copy_region.imageSubresource  = vk::ImageSubresourceLayers{getImageAspectMask(dst_texture->desc.format), p_mip_level, p_base_layer, p_layer_count};
		copy_region.imageOffset       = vk::Offset3D{p_offset.x, p_offset.y, p_offset.z};
		copy_region.imageExtent       = vk::Extent3D{p_extent.x, p_extent.y, p_extent.z};

		vk::CopyBufferToImageInfo2 copy_buffer_to_image_info{};
		copy_buffer_to_image_info.setRegions(copy_region);

		Buffer &src_buffer{p_ctx->buffers[p_src_buffer]};

		copy_buffer_to_image_info.srcBuffer      = src_buffer.buffer;
		copy_buffer_to_image_info.dstImage       = dst_texture->image;
		copy_buffer_to_image_info.dstImageLayout = vk::ImageLayout::eGeneral; // Thank you VK_KHR_unified_image_layouts :)

		cmd.cmd.copyBufferToImage2(copy_buffer_to_image_info);
	}

	auto cmd_fill_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_dst_offset, uint64 p_size, uint32 p_data) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Buffer &buffer{p_ctx->buffers[p_buffer]};

		cmd.cmd.fillBuffer(buffer.buffer, p_dst_offset, p_size, p_data, FunctionDispatcher::get());
	}

	auto cmd_pipeline_barrier(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const BufferMemoryBarrier> p_buffer_memory_barriers,
							  InitialiserList<const TextureMemoryBarrier> p_texture_memory_barriers) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		std::vector<vk::BufferMemoryBarrier2> buffer_barriers(p_buffer_memory_barriers.size());
		for (uint32 i{0u}; i < buffer_barriers.size(); ++i)
		{
			auto &barrier{buffer_barriers[i]};
			barrier.buffer = p_ctx->buffers[p_buffer_memory_barriers[i].buffer].buffer;
			barrier.offset = p_buffer_memory_barriers[i].offset;
			barrier.size   = p_buffer_memory_barriers[i].size;

			barrier.srcStageMask = getVulkanPipelineStageFlags(p_buffer_memory_barriers[i].srcStageMask);
			barrier.dstStageMask = getVulkanPipelineStageFlags(p_buffer_memory_barriers[i].dstStageMask);

			barrier.srcAccessMask = getVulkanAccessFlags(p_buffer_memory_barriers[i].srcAccessMask);
			barrier.dstAccessMask = getVulkanAccessFlags(p_buffer_memory_barriers[i].dstAccessMask);

			barrier.srcQueueFamilyIndex = p_ctx->queueFamilyIndices.get(p_buffer_memory_barriers[i].srcQueue);
			barrier.dstQueueFamilyIndex = p_ctx->queueFamilyIndices.get(p_buffer_memory_barriers[i].dstQueue);
		}

		std::vector<vk::ImageMemoryBarrier2> image_barriers(p_texture_memory_barriers.size());
		for (uint32 i{0u}; i < p_texture_memory_barriers.size(); ++i)
		{
			auto &barrier{image_barriers[i]};
			auto &texture{p_ctx->textures[p_texture_memory_barriers[i].texture]};

			barrier.image = texture.image;

			barrier.oldLayout = vk::ImageLayout::eGeneral;
			barrier.newLayout = vk::ImageLayout::eGeneral;

			barrier.srcStageMask = getVulkanPipelineStageFlags(p_texture_memory_barriers[i].srcStageMask);
			barrier.dstStageMask = getVulkanPipelineStageFlags(p_texture_memory_barriers[i].dstStageMask);

			barrier.srcAccessMask = getVulkanAccessFlags(p_texture_memory_barriers[i].srcAccessMask);
			barrier.dstAccessMask = getVulkanAccessFlags(p_texture_memory_barriers[i].dstAccessMask);

			barrier.subresourceRange.aspectMask     = getImageAspectMask(texture.desc.format);
			barrier.subresourceRange.baseArrayLayer = p_texture_memory_barriers[i].baseArrayLayer;
			barrier.subresourceRange.baseMipLevel   = p_texture_memory_barriers[i].baseMip;
			barrier.subresourceRange.layerCount     = (p_texture_memory_barriers[i].layerCount == UINT32_MAX)
														  ? texture.desc.layerCount
														  : p_texture_memory_barriers[i].layerCount;
			barrier.subresourceRange.levelCount = (p_texture_memory_barriers[i].mipCount == UINT32_MAX) ? texture.desc.mipCount : p_texture_memory_barriers[i].mipCount;

			barrier.srcQueueFamilyIndex = p_ctx->queueFamilyIndices.get(p_texture_memory_barriers[i].srcQueue);
			barrier.dstQueueFamilyIndex = p_ctx->queueFamilyIndices.get(p_texture_memory_barriers[i].dstQueue);
		}

		vk::DependencyInfo dependency_info{};
		dependency_info.setBufferMemoryBarriers(buffer_barriers);
		dependency_info.setImageMemoryBarriers(image_barriers);

		cmd.cmd.pipelineBarrier2(dependency_info, FunctionDispatcher::get());
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

		vk::ImageAspectFlags image_aspect{getImageAspectMask(dst_texture->desc.format)};

		vk::ImageMemoryBarrier2 memory_barrier{};
		memory_barrier.image                           = dst_texture->image;
		memory_barrier.oldLayout                       = vk::ImageLayout::eGeneral;
		memory_barrier.newLayout                       = vk::ImageLayout::eGeneral;
		memory_barrier.srcQueueFamilyIndex             = vk::QueueFamilyIgnored;
		memory_barrier.dstQueueFamilyIndex             = vk::QueueFamilyIgnored;
		memory_barrier.subresourceRange.aspectMask     = image_aspect;
		memory_barrier.subresourceRange.baseArrayLayer = 0u;
		memory_barrier.subresourceRange.layerCount     = 1u;
		memory_barrier.subresourceRange.levelCount     = 1u;

		for (uint32 i{1u}; i < dst_texture->desc.mipCount; ++i)
		{
			std::array<vk::ImageMemoryBarrier2, 2u> pre_blit_barriers;

			// Src mip
			pre_blit_barriers[0u]                               = memory_barrier;
			pre_blit_barriers[0u].subresourceRange.baseMipLevel = i - 1u;
			pre_blit_barriers[0u].srcStageMask                  = vk::PipelineStageFlagBits2::eTransfer;
			pre_blit_barriers[0u].srcAccessMask                 = vk::AccessFlagBits2::eTransferWrite;
			pre_blit_barriers[0u].dstStageMask                  = vk::PipelineStageFlagBits2::eTransfer;
			pre_blit_barriers[0u].dstAccessMask                 = vk::AccessFlagBits2::eTransferRead;

			// Dst mip
			pre_blit_barriers[1u]                               = memory_barrier;
			pre_blit_barriers[1u].subresourceRange.baseMipLevel = i;
			pre_blit_barriers[1u].srcStageMask                  = vk::PipelineStageFlagBits2::eNone;
			pre_blit_barriers[1u].srcAccessMask                 = vk::AccessFlagBits2::eNone;
			pre_blit_barriers[1u].dstStageMask                  = vk::PipelineStageFlagBits2::eTransfer;
			pre_blit_barriers[1u].dstAccessMask                 = vk::AccessFlagBits2::eTransferWrite;

			vk::DependencyInfo dependency_info{};
			dependency_info.imageMemoryBarrierCount = 2u;
			dependency_info.pImageMemoryBarriers    = pre_blit_barriers.data();
			cmd.cmd.pipelineBarrier2(dependency_info);

			std::array<vk::Offset3D, 2> src_offsets;
			std::array<vk::Offset3D, 2> dst_offsets;

			src_offsets[0] = vk::Offset3D{0, 0, 0};
			src_offsets[1] = vk::Offset3D{mip_width, mip_height, 1};

			int32 next_width  = mip_width > 1 ? mip_width / 2 : 1;
			int32 next_height = mip_height > 1 ? mip_height / 2 : 1;

			dst_offsets[0] = vk::Offset3D{0, 0, 0};
			dst_offsets[1] = vk::Offset3D{next_width, next_height, 1};

			vk::ImageBlit2 image_blit{};
			image_blit.srcOffsets     = src_offsets;
			image_blit.dstOffsets     = dst_offsets;
			image_blit.srcSubresource = vk::ImageSubresourceLayers{image_aspect, i - 1, 0, 1};
			image_blit.dstSubresource = vk::ImageSubresourceLayers{image_aspect, i, 0, 1};

			vk::BlitImageInfo2 blit_info{};
			blit_info.srcImage       = dst_texture->image;
			blit_info.dstImage       = dst_texture->image;
			blit_info.srcImageLayout = vk::ImageLayout::eGeneral;
			blit_info.dstImageLayout = vk::ImageLayout::eGeneral;
			blit_info.regionCount    = 1u;
			blit_info.pRegions       = &image_blit;
			blit_info.filter         = vk::Filter::eLinear;
			cmd.cmd.blitImage2(blit_info);

			if (mip_width > 1)
				mip_width /= 2;
			if (mip_height > 1)
				mip_height /= 2;

			vk::ImageMemoryBarrier2 post_blit_barrier       = memory_barrier;
			post_blit_barrier.subresourceRange.baseMipLevel = i - 1u;
			post_blit_barrier.srcStageMask                  = vk::PipelineStageFlagBits2::eTransfer;
			post_blit_barrier.srcAccessMask                 = vk::AccessFlagBits2::eTransferRead;
			post_blit_barrier.dstStageMask                  = vk::PipelineStageFlagBits2::eFragmentShader;
			post_blit_barrier.dstAccessMask                 = vk::AccessFlagBits2::eShaderRead;

			vk::DependencyInfo post_dependency_info{};
			post_dependency_info.imageMemoryBarrierCount = 1u;
			post_dependency_info.pImageMemoryBarriers    = &post_blit_barrier;
			cmd.cmd.pipelineBarrier2(post_dependency_info);
		}

		vk::ImageMemoryBarrier2 final_barrier       = memory_barrier;
		final_barrier.subresourceRange.baseMipLevel = dst_texture->desc.mipCount - 1u;
		final_barrier.srcStageMask                  = vk::PipelineStageFlagBits2::eTransfer;
		final_barrier.srcAccessMask                 = vk::AccessFlagBits2::eTransferWrite;
		final_barrier.dstStageMask                  = vk::PipelineStageFlagBits2::eFragmentShader;
		final_barrier.dstAccessMask                 = vk::AccessFlagBits2::eShaderRead;

		vk::DependencyInfo final_dependency_info{};
		final_dependency_info.imageMemoryBarrierCount = 1u;
		final_dependency_info.pImageMemoryBarriers    = &final_barrier;
		cmd.cmd.pipelineBarrier2(final_dependency_info);
	}

	auto cmd_begin_rendering(GPUContext *p_ctx, CommandListHandle p_cmd_list, const RenderingInfo &p_rendering_info) -> void
	{
		std::vector<vk::RenderingAttachmentInfo> colour_attachments(p_rendering_info.colourAttachments.size());
		for (uint32 i{0u}; i < p_rendering_info.colourAttachments.size(); ++i)
		{
			auto &src_attachment{p_rendering_info.colourAttachments[i]};
			auto &dst_attachment{colour_attachments[i]};

			const Texture *render_target{p_ctx->textures.tryGet(src_attachment.renderTarget)};
			TST_ASSERT(render_target);
			dst_attachment.imageView   = render_target->imageView;
			dst_attachment.imageLayout = vk::ImageLayout::eGeneral;

			dst_attachment.loadOp     = getLoadOp(src_attachment.usageOp);
			dst_attachment.storeOp    = getStoreOp(src_attachment.usageOp);
			dst_attachment.clearValue = *reinterpret_cast<const vk::ClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (p_ctx->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture *resolve_render_target{p_ctx->textures.tryGet(src_attachment.resolveTarget)};
				TST_ASSERT(resolve_render_target);
				dst_attachment.resolveImageView   = resolve_render_target->imageView;
				dst_attachment.resolveImageLayout = vk::ImageLayout::eGeneral;
				dst_attachment.resolveMode        = getResolveMode(src_attachment.resolveMode);
			}
		}

		vk::RenderingAttachmentInfo depth_attachment{};
		if (p_rendering_info.depthAttachment.has_value())
		{
			auto &src_attachment{p_rendering_info.depthAttachment.value()};

			const Texture *render_target{p_ctx->textures.tryGet(src_attachment.renderTarget)};
			TST_ASSERT(render_target);
			depth_attachment.imageView   = render_target->imageView;
			depth_attachment.imageLayout = vk::ImageLayout::eGeneral;

			depth_attachment.loadOp     = getLoadOp(src_attachment.usageOp);
			depth_attachment.storeOp    = getStoreOp(src_attachment.usageOp);
			depth_attachment.clearValue = *reinterpret_cast<const vk::ClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (p_ctx->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture *resolve_render_target{p_ctx->textures.tryGet(src_attachment.resolveTarget)};
				TST_ASSERT(resolve_render_target);
				depth_attachment.resolveImageView   = resolve_render_target->imageView;
				depth_attachment.resolveImageLayout = vk::ImageLayout::eGeneral;
				depth_attachment.resolveMode        = getResolveMode(src_attachment.resolveMode);
			}
		}

		vk::RenderingAttachmentInfo stencil_attachment{};
		if (p_rendering_info.stencilAttachment.has_value())
		{
			auto &src_attachment{p_rendering_info.stencilAttachment.value()};

			const Texture *render_target{p_ctx->textures.tryGet(src_attachment.renderTarget)};
			TST_ASSERT(render_target);
			stencil_attachment.imageView   = render_target->imageView;
			stencil_attachment.imageLayout = vk::ImageLayout::eGeneral;

			stencil_attachment.loadOp     = getLoadOp(src_attachment.usageOp);
			stencil_attachment.storeOp    = getStoreOp(src_attachment.usageOp);
			stencil_attachment.clearValue = *reinterpret_cast<const vk::ClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (p_ctx->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture *resolve_render_target{p_ctx->textures.tryGet(src_attachment.resolveTarget)};
				TST_ASSERT(resolve_render_target);
				stencil_attachment.resolveImageView   = resolve_render_target->imageView;
				stencil_attachment.resolveImageLayout = vk::ImageLayout::eGeneral;
				stencil_attachment.resolveMode        = getResolveMode(src_attachment.resolveMode);
			}
		}

		vk::RenderingInfo rendering_info{};
		rendering_info.renderArea = vk::Rect2D{
			vk::Offset2D{p_rendering_info.renderArea.offset.x, p_rendering_info.renderArea.offset.y},
			vk::Extent2D{p_rendering_info.renderArea.size.x, p_rendering_info.renderArea.size.y}
		};
		rendering_info.layerCount           = 1u;
		rendering_info.viewMask             = 0u; // I don't even know what ts is
		rendering_info.colorAttachmentCount = colour_attachments.size();
		rendering_info.pColorAttachments    = colour_attachments.data();
		rendering_info.pDepthAttachment     = p_rendering_info.depthAttachment.has_value() ? &depth_attachment : nullptr;
		rendering_info.pStencilAttachment   = p_rendering_info.stencilAttachment.has_value() ? &stencil_attachment : nullptr;
		rendering_info.flags                = vk::RenderingFlagBits::eContentsSecondaryCommandBuffers;

		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.beginRendering(rendering_info, FunctionDispatcher::get());
	}

	auto cmd_end_rendering(GPUContext *p_ctx, CommandListHandle p_cmd_list) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.endRendering(FunctionDispatcher::get());
	}

	auto cmd_push_data(GPUContext *p_ctx, CommandListHandle p_cmd_list, const void *p_data, uint64 p_size, uint32 p_offset) -> void
	{
		vk::PushDataInfoEXT push_data_info{};
		push_data_info.data.address = p_data;
		push_data_info.data.size    = p_size;
		push_data_info.offset       = p_offset;

		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.pushDataEXT(push_data_info, FunctionDispatcher::get());
	}

	auto cmd_bind_shaders(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const ShaderHandle> p_shaders) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		std::unordered_map<vk::ShaderStageFlagBits, vk::ShaderEXT> shader_stage_map{
			{vk::ShaderStageFlagBits::eVertex, nullptr},
			{vk::ShaderStageFlagBits::eGeometry, nullptr},
			{vk::ShaderStageFlagBits::eFragment, nullptr},
			{vk::ShaderStageFlagBits::eCompute, nullptr},
			{vk::ShaderStageFlagBits::eMeshEXT, nullptr},
			{vk::ShaderStageFlagBits::eTaskEXT, nullptr}
		};
		for (uint32 i{0u}; i < p_shaders.size(); ++i)
		{
			const Shader &shader{p_ctx->shaders[p_shaders[i]]};
			shader_stage_map[shader.stage] = shader.shader;
		}

		std::vector<vk::ShaderStageFlagBits> stages{shader_stage_map | std::views::keys | std::ranges::to<std::vector>()};
		std::vector<vk::ShaderEXT>           shaders{shader_stage_map | std::views::values | std::ranges::to<std::vector>()};
		cmd.cmd.bindShadersEXT(stages, shaders, FunctionDispatcher::get());
	}

	auto cmd_bind_resource_heap(GPUContext *p_ctx, CommandListHandle p_cmd_list, ResourceDescriptorHeapHandle p_resource_heap) -> void
	{
		CommandList &           cmd{p_ctx->commandLists[p_cmd_list]};
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};
		cmd.cmd.bindResourceHeapEXT(resource_heap.bindInfo, FunctionDispatcher::get());
	}

	auto cmd_bind_sampler_heap(GPUContext *p_ctx, CommandListHandle p_cmd_list, SamplerDescriptorHeapHandle p_sampler_heap) -> void
	{
		CommandList &          cmd{p_ctx->commandLists[p_cmd_list]};
		SamplerDescriptorHeap &sampler_heap{p_ctx->samplerHeaps[p_sampler_heap]};
		cmd.cmd.bindSamplerHeapEXT(sampler_heap.bindInfo, FunctionDispatcher::get());
	}

	auto cmd_bind_index_buffer(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_index_buffer, EIndexType p_index_type) -> void
	{
		CommandList & cmd{p_ctx->commandLists[p_cmd_list]};
		const Buffer *index_buffer{p_ctx->buffers.tryGet(p_index_buffer)};

		cmd.cmd.setVertexInputEXT({}, {}, FunctionDispatcher::get());

		vk::IndexType index_type{};
		switch (p_index_type)
		{
			case EIndexType::eUint8: index_type = vk::IndexType::eUint8;
				break;
			case EIndexType::eUint16: index_type = vk::IndexType::eUint16;
				break;
			case EIndexType::eUint32: index_type = vk::IndexType::eUint32;
				break;
		}
		if (index_buffer)
			cmd.cmd.bindIndexBuffer(index_buffer->buffer, 0u, index_type, FunctionDispatcher::get());
	}

	auto cmd_set_primitive_topology(GPUContext *p_ctx, CommandListHandle p_cmd_list, EPrimitiveTopology p_primitive_topology) -> void
	{
		vk::PrimitiveTopology primitive_topology{};
		switch (p_primitive_topology)
		{
			case EPrimitiveTopology::ePointList: primitive_topology = vk::PrimitiveTopology::ePointList;
				break;
			case EPrimitiveTopology::eLineList: primitive_topology = vk::PrimitiveTopology::eLineList;
				break;
			case EPrimitiveTopology::eLineStrip: primitive_topology = vk::PrimitiveTopology::eLineStrip;
				break;
			case EPrimitiveTopology::eTriangleList: primitive_topology = vk::PrimitiveTopology::eTriangleList;
				break;
			case EPrimitiveTopology::eTriangleStrip: primitive_topology = vk::PrimitiveTopology::eTriangleStrip;
				break;
			case EPrimitiveTopology::eTriangleFan: primitive_topology = vk::PrimitiveTopology::eTriangleFan;
				break;
			case EPrimitiveTopology::eLineListWithAdjacency: primitive_topology = vk::PrimitiveTopology::eLineListWithAdjacency;
				break;
			case EPrimitiveTopology::eLineStripWithAdjacency: primitive_topology = vk::PrimitiveTopology::eLineStripWithAdjacency;
				break;
			case EPrimitiveTopology::eTriangleListWithAdjacency: primitive_topology = vk::PrimitiveTopology::eTriangleListWithAdjacency;
				break;
			case EPrimitiveTopology::eTriangleStripWithAdjacency: primitive_topology = vk::PrimitiveTopology::eTriangleStripWithAdjacency;
				break;
			case EPrimitiveTopology::ePatchList: primitive_topology = vk::PrimitiveTopology::ePatchList;
				break;
		}
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setPrimitiveTopologyEXT(primitive_topology, FunctionDispatcher::get());
	}

	auto cmd_set_primitive_restart(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable, uint32 p_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setPrimitiveRestartEnableEXT(p_enable, FunctionDispatcher::get());
		if (p_enable)
			cmd.cmd.setPrimitiveRestartIndexEXT(p_index, FunctionDispatcher::get());
	}

	auto cmd_set_viewport(GPUContext *p_ctx, CommandListHandle p_cmd_list, const math::Viewport &p_viewport) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setViewportWithCountEXT(vk::Viewport{
											p_viewport.offset.x,
											p_viewport.offset.y,
											p_viewport.size.x,
											p_viewport.size.y,
											p_viewport.depthBounds.x,
											p_viewport.depthBounds.y
										}, FunctionDispatcher::get());
	}

	auto cmd_set_scissor(GPUContext *p_ctx, CommandListHandle p_cmd_list, const math::Rect &p_scissor_rect) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setScissorWithCountEXT(vk::Rect2D{{p_scissor_rect.offset.x, p_scissor_rect.offset.y}, {p_scissor_rect.size.x, p_scissor_rect.size.y}},
									   FunctionDispatcher::get());
	}

	auto cmd_set_rasterizer_discard_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setRasterizerDiscardEnableEXT(p_enable, FunctionDispatcher::get());
	}

	auto cmd_set_polygon_mode(GPUContext *p_ctx, CommandListHandle p_cmd_list, EPolygonMode p_polygon_mode) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vk::PolygonMode polygon_mode{};
		switch (p_polygon_mode)
		{
			case EPolygonMode::eFill: polygon_mode = vk::PolygonMode::eFill;
				break;
			case EPolygonMode::eLine: polygon_mode = vk::PolygonMode::eLine;
				break;
			case EPolygonMode::ePoint: polygon_mode = vk::PolygonMode::ePoint;
				break;
		}
		cmd.cmd.setPolygonModeEXT(polygon_mode, FunctionDispatcher::get());
	}

	auto cmd_set_cull_mode(GPUContext *p_ctx, CommandListHandle p_cmd_list, ECullMode p_cull_mode) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vk::CullModeFlags cull_mode{};
		switch (p_cull_mode)
		{
			case ECullMode::eNone: cull_mode = vk::CullModeFlagBits::eNone;
				break;
			case ECullMode::eFront: cull_mode = vk::CullModeFlagBits::eFront;
				break;
			case ECullMode::eBack: cull_mode = vk::CullModeFlagBits::eBack;
				break;
			case ECullMode::eFrontAndBack: cull_mode = vk::CullModeFlagBits::eFrontAndBack;
				break;
		}
		cmd.cmd.setCullModeEXT(cull_mode, FunctionDispatcher::get());
	}

	auto cmd_set_front_face(GPUContext *p_ctx, CommandListHandle p_cmd_list, EFrontFace p_front_face) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vk::FrontFace front_face{(p_front_face == EFrontFace::eCW) ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise};
		cmd.cmd.setFrontFaceEXT(front_face, FunctionDispatcher::get());
	}

	auto cmd_set_depth_bias(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_enable, float32 p_constant_factor, float32 p_clamp, float32 p_slope_factor) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		cmd.cmd.setDepthBiasEnableEXT(p_enable, FunctionDispatcher::get());
		if (p_enable)
			cmd.cmd.setDepthBias2EXT(vk::DepthBiasInfoEXT{p_constant_factor, p_clamp, p_slope_factor}, FunctionDispatcher::get());
	}

	auto cmd_set_line_width(GPUContext *p_ctx, CommandListHandle p_cmd_list, float32 p_line_width) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setLineWidth(p_line_width, FunctionDispatcher::get());
	}

	auto cmd_set_rasterization_samples(GPUContext *p_ctx, CommandListHandle p_cmd_list, ESampleCount p_sample_count) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		vk::SampleCountFlagBits sample_count{getVulkanSampleCount(p_sample_count)};
		cmd.cmd.setRasterizationSamplesEXT(sample_count, FunctionDispatcher::get());

		cmd.cmd.setSampleMaskEXT(sample_count, 0xFFFFFFFF, FunctionDispatcher::get()); // I don't think I should expose this
		cmd.cmd.setAlphaToCoverageEnableEXT(false, FunctionDispatcher::get());         // I don't think I should expose this either
	}

	auto cmd_set_depth_state(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_test_enable, bool p_write_enable, bool p_clamp_enable,
							 ECompareOp  p_compare_op) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setDepthTestEnableEXT(p_test_enable, FunctionDispatcher::get());
		if (p_test_enable)
		{
			cmd.cmd.setDepthWriteEnableEXT(p_write_enable, FunctionDispatcher::get());
			cmd.cmd.setDepthClampEnableEXT(p_clamp_enable, FunctionDispatcher::get());

			vk::CompareOp compare_op{};
			switch (p_compare_op)
			{
				case ECompareOp::eNever: compare_op = vk::CompareOp::eNever;
					break;
				case ECompareOp::eLess: compare_op = vk::CompareOp::eLess;
					break;
				case ECompareOp::eEqual: compare_op = vk::CompareOp::eEqual;
					break;
				case ECompareOp::eLessOrEqual: compare_op = vk::CompareOp::eLessOrEqual;
					break;
				case ECompareOp::eGreater: compare_op = vk::CompareOp::eGreater;
					break;
				case ECompareOp::eNotEqual: compare_op = vk::CompareOp::eNotEqual;
					break;
				case ECompareOp::eGreaterOrEqual: compare_op = vk::CompareOp::eGreaterOrEqual;
					break;
				case ECompareOp::eAlways: compare_op = vk::CompareOp::eAlways;
					break;
			}
			cmd.cmd.setDepthCompareOpEXT(compare_op, FunctionDispatcher::get());
		}
	}

	auto cmd_set_stencil_state(GPUContext *p_ctx, CommandListHandle p_cmd_list, bool p_test_enable) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setStencilTestEnableEXT(p_test_enable, FunctionDispatcher::get());
	}

	auto cmd_set_colour_write_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const bool32> p_enables) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setColorWriteEnableEXT(p_enables, FunctionDispatcher::get());
	}

	auto cmd_set_colour_write_mask(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const EColourComponentFlags> p_masks,
								   uint32      p_first_attachment_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const auto get_vulkan_colour_component_flags{
			+[](const EColourComponentFlags p_flags) -> vk::ColorComponentFlags
			{
				vk::ColorComponentFlags out_flags{0u};
				out_flags |= (p_flags & EColourComponentFlagBits::eR) ? vk::ColorComponentFlagBits::eR : vk::ColorComponentFlagBits{0u};
				out_flags |= (p_flags & EColourComponentFlagBits::eG) ? vk::ColorComponentFlagBits::eG : vk::ColorComponentFlagBits{0u};
				out_flags |= (p_flags & EColourComponentFlagBits::eB) ? vk::ColorComponentFlagBits::eB : vk::ColorComponentFlagBits{0u};
				out_flags |= (p_flags & EColourComponentFlagBits::eA) ? vk::ColorComponentFlagBits::eA : vk::ColorComponentFlagBits{0u};
				return out_flags;
			}
		};

		std::vector<vk::ColorComponentFlags> component_flags(p_masks.size());
		for (uint32 i{0u}; i < p_masks.size(); ++i)
			component_flags[i] = get_vulkan_colour_component_flags(p_masks[i]);
		cmd.cmd.setColorWriteMaskEXT(p_first_attachment_index, component_flags, FunctionDispatcher::get());
	}

	auto cmd_set_colour_blend_enable(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const bool32> p_enables, uint32 p_first_attachment_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.setColorBlendEnableEXT(p_first_attachment_index, p_enables, FunctionDispatcher::get());
	}

	auto cmd_set_colour_blend_equation(GPUContext *p_ctx, CommandListHandle p_cmd_list, InitialiserList<const ColourBlendEquation> p_blend_equations,
									   uint32      p_first_attachment_index) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		const auto get_colour_blend_factor{
			+[](EBlendFactor p_factor) -> vk::BlendFactor
			{
				switch (p_factor)
				{
					case EBlendFactor::eZero: return vk::BlendFactor::eZero;
					case EBlendFactor::eOne: return vk::BlendFactor::eOne;
					case EBlendFactor::eSrcColor: return vk::BlendFactor::eSrcColor;
					case EBlendFactor::eOneMinusSrcColor: return vk::BlendFactor::eOneMinusSrcColor;
					case EBlendFactor::eDstColor: return vk::BlendFactor::eDstColor;
					case EBlendFactor::eOneMinusDstColor: return vk::BlendFactor::eOneMinusDstColor;
					case EBlendFactor::eSrcAlpha: return vk::BlendFactor::eSrcAlpha;
					case EBlendFactor::eOneMinusSrcAlpha: return vk::BlendFactor::eOneMinusSrcAlpha;
					case EBlendFactor::eDstAlpha: return vk::BlendFactor::eDstAlpha;
					case EBlendFactor::eOneMinusDstAlpha: return vk::BlendFactor::eOneMinusDstAlpha;
					case EBlendFactor::eConstantColor: return vk::BlendFactor::eConstantColor;
					case EBlendFactor::eOneMinusConstantColor: return vk::BlendFactor::eOneMinusConstantColor;
					case EBlendFactor::eConstantAlpha: return vk::BlendFactor::eConstantAlpha;
					case EBlendFactor::eOneMinusConstantAlpha: return vk::BlendFactor::eOneMinusConstantAlpha;
					case EBlendFactor::eSrcAlphaSaturate: return vk::BlendFactor::eSrcAlphaSaturate;
					case EBlendFactor::eSrc1Color: return vk::BlendFactor::eSrc1Color;
					case EBlendFactor::eOneMinusSrc1Color: return vk::BlendFactor::eOneMinusSrc1Color;
					case EBlendFactor::eSrc1Alpha: return vk::BlendFactor::eSrc1Alpha;
					case EBlendFactor::eOneMinusSrc1Alpha: return vk::BlendFactor::eOneMinusSrc1Alpha;
				}
				return vk::BlendFactor::eZero;
			}
		};

		const auto get_colour_blend_op{
			+[](EBlendOp p_op) -> vk::BlendOp
			{
				switch (p_op)
				{
					case EBlendOp::eAdd: return vk::BlendOp::eAdd;
					case EBlendOp::eSubtract: return vk::BlendOp::eSubtract;
					case EBlendOp::eReverseSubtract: return vk::BlendOp::eReverseSubtract;
					case EBlendOp::eMin: return vk::BlendOp::eMin;
					case EBlendOp::eMax: return vk::BlendOp::eMax;
				}
				return vk::BlendOp::eAdd;
			}
		};
		std::vector<vk::ColorBlendEquationEXT> colour_blend_equations(p_blend_equations.size());
		for (uint32 i{0u}; i < p_blend_equations.size(); ++i)
		{
			auto &dst_equation{colour_blend_equations[i]};
			dst_equation.srcColorBlendFactor = get_colour_blend_factor(p_blend_equations[i].srcColorBlendFactor);
			dst_equation.dstColorBlendFactor = get_colour_blend_factor(p_blend_equations[i].dstColorBlendFactor);
			dst_equation.colorBlendOp        = get_colour_blend_op(p_blend_equations[i].colorBlendOp);
			dst_equation.srcAlphaBlendFactor = get_colour_blend_factor(p_blend_equations[i].srcAlphaBlendFactor);
			dst_equation.dstAlphaBlendFactor = get_colour_blend_factor(p_blend_equations[i].dstAlphaBlendFactor);
			dst_equation.alphaBlendOp        = get_colour_blend_op(p_blend_equations[i].alphaBlendOp);
		}
		cmd.cmd.setColorBlendEquationEXT(p_first_attachment_index, colour_blend_equations, FunctionDispatcher::get());
	}

	auto cmd_draw(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_vertex_count, uint32 p_instance_count, uint32 p_first_vertex, uint32 p_first_instance) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.draw(p_vertex_count, p_instance_count, p_first_vertex, p_first_instance, FunctionDispatcher::get());
	}

	auto cmd_draw_indexed(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_index_count, uint32 p_instance_count, uint32 p_first_index, int32 p_vertex_offset,
						  uint32      p_first_instance) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.drawIndexed(p_index_count, p_instance_count, p_first_index, p_vertex_offset, p_first_instance, FunctionDispatcher::get());
	}

	auto cmd_draw_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count, uint32 p_stride) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		Buffer &     buffer{p_ctx->buffers[p_buffer]};
		cmd.cmd.drawIndirect(buffer.buffer, p_offset, p_draw_count, p_stride, FunctionDispatcher::get());
	}

	auto cmd_draw_indexed_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count, uint32 p_stride) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		Buffer &     buffer{p_ctx->buffers[p_buffer]};
		cmd.cmd.drawIndexedIndirect(buffer.buffer, p_offset, p_draw_count, p_stride, FunctionDispatcher::get());
	}

	auto cmd_draw_mesh_tasks(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_group_count_x, uint32 p_group_count_y, uint32 p_group_count_z) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.drawMeshTasksEXT(p_group_count_x, p_group_count_y, p_group_count_z, FunctionDispatcher::get());
	}

	auto cmd_draw_mesh_tasks_indirect(GPUContext *p_ctx, CommandListHandle p_cmd_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count,
									  uint32      p_stride) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		Buffer &     buffer{p_ctx->buffers[p_buffer]};
		cmd.cmd.drawMeshTasksIndirectEXT(buffer.buffer, p_offset, p_draw_count, p_stride, FunctionDispatcher::get());
	}

	auto cmd_draw_mesh_tasks_indirect_count(GPUContext * p_ctx, CommandListHandle p_cmd_list, BufferHandle      p_indirect_buffer, uint64 p_indirect_buffer_offset,
											BufferHandle p_count_buffer, uint64   p_count_buffer_offset, uint32 p_max_draw_count, uint32  p_stride) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};

		Buffer &indirect_buffer{p_ctx->buffers[p_indirect_buffer]};
		Buffer &count_buffer{p_ctx->buffers[p_count_buffer]};

		cmd.cmd.drawMeshTasksIndirectCountEXT(indirect_buffer.buffer, p_indirect_buffer_offset, count_buffer.buffer, p_count_buffer_offset, p_max_draw_count, p_stride,
											  FunctionDispatcher::get());
	}

	auto cmd_dispatch(GPUContext *p_ctx, CommandListHandle p_cmd_list, uint32 p_work_groups_x, uint32 p_work_groups_y, uint32 p_work_groups_z) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		cmd.cmd.dispatch(p_work_groups_x, p_work_groups_y, p_work_groups_z);
	}

	static auto getTextureBaseHeapSlot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};
		return p_heap_slot + (resource_heap.imageSegmentOffset / resource_heap.imageDescriptorSize);
	}

	static auto getSegmentRelativeTextureHeapSlot(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> uint32
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

		vk::DeviceSize resource_heap_size{
			TST_ALIGN(resource_heap.bufferSegmentSize + resource_heap.imageSegmentSize + heap_props.minResourceHeapReservedRange, heap_props.resourceHeapAlignment)
		};

		vk::BufferCreateInfo resource_heap_create_info{};
		resource_heap_create_info.usage       = vk::BufferUsageFlagBits::eDescriptorHeapEXT | vk::BufferUsageFlagBits::eShaderDeviceAddress;
		resource_heap_create_info.sharingMode = vk::SharingMode::eExclusive;
		resource_heap_create_info.size        = resource_heap_size;

		VmaAllocationCreateInfo heap_allocation_create_info{};
		heap_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		heap_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		vmaCreateBuffer(p_ctx->allocator, reinterpret_cast<VkBufferCreateInfo *>(&resource_heap_create_info), &heap_allocation_create_info,
						reinterpret_cast<VkBuffer *>(&resource_heap.heapBuffer), &resource_heap.heapAllocation, nullptr);

		vmaMapMemory(p_ctx->allocator, resource_heap.heapAllocation, &resource_heap.mappedHeapMemory);

		vk::DeviceAddressRangeKHR device_address_range{};
		device_address_range.address               = p_ctx->logicalDevice.getBufferAddress({resource_heap.heapBuffer});
		device_address_range.size                  = resource_heap_size;
		resource_heap.bindInfo.heapRange           = device_address_range;
		resource_heap.bindInfo.reservedRangeOffset = resource_heap_size - heap_props.minResourceHeapReservedRange;
		resource_heap.bindInfo.reservedRangeSize   = heap_props.minResourceHeapReservedRange;

		return p_ctx->resourceHeaps.emplace(std::move(resource_heap));
	}

	auto sampler_descriptor_heap_create(GPUContext *p_ctx, const SamplerDescriptorHeapDesc &p_desc) -> SamplerDescriptorHeapHandle
	{
		const auto &heap_props{p_ctx->descriptorHeapProperties};

		SamplerDescriptorHeap sampler_heap{};

		mtx_init(&sampler_heap.samplerSlotMutex, mtx_plain);

		sampler_heap.samplerSlotAllocator = {p_desc.maxSamplerDescriptors};

		sampler_heap.samplerDescriptorSize = TST_ALIGN(heap_props.samplerDescriptorSize, heap_props.samplerDescriptorAlignment);

		vk::DeviceSize sampler_heap_size{
			TST_ALIGN(p_desc.maxSamplerDescriptors * sampler_heap.samplerDescriptorSize + heap_props.minResourceHeapReservedRange, heap_props.resourceHeapAlignment)
		};

		vk::BufferCreateInfo sampler_heap_create_info{};
		sampler_heap_create_info.usage       = vk::BufferUsageFlagBits::eDescriptorHeapEXT | vk::BufferUsageFlagBits::eShaderDeviceAddress;
		sampler_heap_create_info.sharingMode = vk::SharingMode::eExclusive;
		sampler_heap_create_info.size        = sampler_heap_size;

		VmaAllocationCreateInfo heap_allocation_create_info{};
		heap_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		heap_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		vmaCreateBuffer(p_ctx->allocator, reinterpret_cast<VkBufferCreateInfo *>(&sampler_heap_create_info), &heap_allocation_create_info,
						reinterpret_cast<VkBuffer *>(&sampler_heap.heapBuffer), &sampler_heap.heapAllocation, nullptr);

		vmaMapMemory(p_ctx->allocator, sampler_heap.heapAllocation, &sampler_heap.mappedHeapMemory);

		vk::DeviceAddressRangeKHR device_address_range{};
		device_address_range.address              = p_ctx->logicalDevice.getBufferAddress({sampler_heap.heapBuffer});
		device_address_range.size                 = sampler_heap_size;
		sampler_heap.bindInfo.heapRange           = device_address_range;
		sampler_heap.bindInfo.reservedRangeOffset = sampler_heap_size - heap_props.minSamplerHeapReservedRange;
		sampler_heap.bindInfo.reservedRangeSize   = heap_props.minSamplerHeapReservedRange;

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

		return getTextureBaseHeapSlot(p_ctx, p_resource_heap, slot);
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
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		const uint32 real_heap_slot{getSegmentRelativeTextureHeapSlot(p_ctx, p_resource_heap, p_heap_slot)};

		mtx_lock(&resource_heap.imageSlotMutex);
		resource_heap.imageSlotAllocator.freeSlot(real_heap_slot);
		mtx_unlock(&resource_heap.imageSlotMutex);
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

		vk::DeviceAddressRangeKHR device_address_range{buffer.address, buffer.size};

		vk::HostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(resource_heap.mappedHeapMemory) + static_cast<uint64>(p_heap_slot) * resource_heap.
													  bufferDescriptorSize);
		host_range.size = resource_heap.bufferDescriptorSize;

		vk::DescriptorType descriptor_type{}; // I won't allow a buffer that is both uniform and storage
		if (buffer.usageFlags & EBufferUsageFlagBits::eUniformBuffer)
			descriptor_type = vk::DescriptorType::eUniformBuffer;
		else if (buffer.usageFlags & EBufferUsageFlagBits::eStorageBuffer)
			descriptor_type = vk::DescriptorType::eStorageBuffer;
		else
		{
			TST_PERMA_ASSERT_MSG(false, "What type of buffer is ts?!");
		}

		vk::ResourceDescriptorInfoEXT resource_info{};
		resource_info.type               = descriptor_type;
		resource_info.data.pAddressRange = &device_address_range;

		p_ctx->logicalDevice.writeResourceDescriptorsEXT(resource_info, host_range, FunctionDispatcher::get());
	}

	auto resource_descriptor_heap_write_texture(GPUContext *p_ctx, ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, TextureHandle p_texture,
												bool        p_storage, uint32                   p_mip) -> void
	{
		ResourceDescriptorHeap &resource_heap{p_ctx->resourceHeaps[p_resource_heap]};

		Texture *texture{p_ctx->textures.tryGet(p_texture)};
		TST_ASSERT(texture);

		vk::ImageViewType image_view_type{vk::ImageViewType::e2D};

		switch (texture->desc.type)
		{
			case ETextureType::e1D:
				image_view_type = vk::ImageViewType::e1D;
				break;
			case ETextureType::e2D:
				image_view_type = vk::ImageViewType::e2D;
				break;
			case ETextureType::e3D:
				image_view_type = vk::ImageViewType::e3D;
				break;
			case ETextureType::eCube:
				image_view_type = vk::ImageViewType::eCube;
				break;
		}

		vk::Format image_format{getVulkanFormat(texture->desc.format)};

		vk::ImageViewCreateInfo image_view_create_info{};
		image_view_create_info.image      = texture->image;
		image_view_create_info.viewType   = image_view_type;
		image_view_create_info.format     = image_format;
		image_view_create_info.components = vk::ComponentMapping{
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		};
		image_view_create_info.subresourceRange = vk::ImageSubresourceRange{
			getImageAspectMask(texture->desc.format),
			(p_mip == UINT32_MAX) ? 0u : p_mip,
			(p_mip == UINT32_MAX) ? texture->desc.mipCount : 1u,
			0u,
			texture->desc.layerCount
		};

		uint32 real_heap_slot{getSegmentRelativeTextureHeapSlot(p_ctx, p_resource_heap, p_heap_slot)};

		vk::HostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(resource_heap.mappedHeapMemory) + resource_heap.imageSegmentOffset + static_cast<uint64>(
														  real_heap_slot) * resource_heap.imageDescriptorSize);
		host_range.size = resource_heap.imageDescriptorSize;

		vk::ImageDescriptorInfoEXT image_info{};
		image_info.pView  = &image_view_create_info;
		image_info.layout = vk::ImageLayout::eGeneral;

		vk::ResourceDescriptorInfoEXT resource_info{};
		resource_info.data.pImage = &image_info;
		resource_info.type        = (p_storage ? vk::DescriptorType::eStorageImage : vk::DescriptorType::eSampledImage);

		p_ctx->logicalDevice.writeResourceDescriptorsEXT(resource_info, host_range, FunctionDispatcher::get());
	}

	auto sampler_descriptor_heap_write_sampler(GPUContext *p_ctx, SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot, SamplerHandle p_sampler) -> void
	{
		SamplerDescriptorHeap &sampler_heap{p_ctx->samplerHeaps[p_sampler_heap]};
		Sampler &              sampler{p_ctx->samplers[p_sampler]};

		constexpr auto getAddressMode{
			+[](ESamplerAddressMode p_address_mode) -> vk::SamplerAddressMode
			{
				switch (p_address_mode)
				{
					case ESamplerAddressMode::eRepeat: return vk::SamplerAddressMode::eRepeat;
					case ESamplerAddressMode::eMirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
					case ESamplerAddressMode::eClampToEdge: return vk::SamplerAddressMode::eClampToEdge;
					case ESamplerAddressMode::eClampToBorder: return vk::SamplerAddressMode::eClampToBorder;
				}
				return vk::SamplerAddressMode::eRepeat;
			}
		};

		vk::SamplerCreateInfo sampler_create_info{};
		sampler_create_info.minFilter    = sampler.desc.minFilter == EFilter::eLinear ? vk::Filter::eLinear : vk::Filter::eNearest;
		sampler_create_info.magFilter    = sampler.desc.magFilter == EFilter::eLinear ? vk::Filter::eLinear : vk::Filter::eNearest;
		sampler_create_info.mipmapMode   = sampler.desc.mipmapMode == ESamplerMipmapMode::eLinear ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest;
		sampler_create_info.addressModeU = getAddressMode(sampler.desc.addressModeU);
		sampler_create_info.addressModeV = getAddressMode(sampler.desc.addressModeV);
		sampler_create_info.addressModeW = getAddressMode(sampler.desc.addressModeW);

		sampler_create_info.mipLodBias       = 0.0f;
		sampler_create_info.anisotropyEnable = true;
		sampler_create_info.maxAnisotropy    = (sampler.desc.maxAnisotropy == -1.0f)
												   ? p_ctx->physicalDevice.getProperties2().properties.limits.maxSamplerAnisotropy
												   : sampler.desc.maxAnisotropy;
		sampler_create_info.compareEnable           = false;
		sampler_create_info.compareOp               = vk::CompareOp::eNever;
		sampler_create_info.minLod                  = sampler.desc.minLod;
		sampler_create_info.maxLod                  = sampler.desc.maxLod;
		sampler_create_info.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
		sampler_create_info.unnormalizedCoordinates = false;

		vk::HostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(sampler_heap.mappedHeapMemory) + static_cast<uint64>(p_heap_slot) * sampler_heap.
													  samplerDescriptorSize);
		host_range.size = sampler_heap.samplerDescriptorSize;

		p_ctx->logicalDevice.writeSamplerDescriptorsEXT(sampler_create_info, host_range, FunctionDispatcher::get());
	}

	auto buffer_create(GPUContext *p_ctx, const BufferDesc &p_desc) -> BufferHandle
	{
		vk::BufferUsageFlags buffer_usage_flags{0u};

		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eTransferSrc) ? vk::BufferUsageFlagBits::eTransferSrc : vk::BufferUsageFlagBits{0u};
		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eTransferDst) ? vk::BufferUsageFlagBits::eTransferDst : vk::BufferUsageFlagBits{0u};
		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eVertexBuffer) ? vk::BufferUsageFlagBits::eVertexBuffer : vk::BufferUsageFlagBits{0u};
		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eIndexBuffer) ? vk::BufferUsageFlagBits::eIndexBuffer : vk::BufferUsageFlagBits{0u};
		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eIndirectBuffer) ? vk::BufferUsageFlagBits::eIndirectBuffer : vk::BufferUsageFlagBits{0u};
		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eUniformBuffer)
								  ? vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
								  : vk::BufferUsageFlagBits{0u};
		buffer_usage_flags |= (p_desc.usage & EBufferUsageFlagBits::eStorageBuffer)
								  ? vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
								  : vk::BufferUsageFlagBits{0u};

		vk::BufferCreateInfo buffer_create_info{};
		buffer_create_info.size  = p_desc.size;
		buffer_create_info.usage = buffer_usage_flags;

		std::array<uint32, 2u> queue_family_indices{p_ctx->queueFamilyIndices.graphics, p_ctx->queueFamilyIndices.transfer};
		if (queue_family_indices[0u] != queue_family_indices[1u])
		{
			buffer_create_info.sharingMode           = vk::SharingMode::eConcurrent;
			buffer_create_info.queueFamilyIndexCount = queue_family_indices.size();
			buffer_create_info.pQueueFamilyIndices   = queue_family_indices.data();
		}
		else
		{
			buffer_create_info.sharingMode = vk::SharingMode::eExclusive;
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

		vk::Buffer    buffer{nullptr};
		VmaAllocation allocation{nullptr};

		VmaAllocationInfo allocation_info{};
		vmaCreateBuffer(p_ctx->allocator, reinterpret_cast<const VkBufferCreateInfo *>(&buffer_create_info), &allocation_create_info,
						reinterpret_cast<VkBuffer *>(&buffer), &allocation, &allocation_info);

		DeviceAddress device_address{0u};
		if (buffer_usage_flags & vk::BufferUsageFlagBits::eShaderDeviceAddress)
			device_address = p_ctx->logicalDevice.getBufferAddress({buffer});

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
			return {};
		}

		HWND hwnd{static_cast<HWND>(p_hwnd)};

		vk::Win32SurfaceCreateInfoKHR surface_create_info{};
		surface_create_info.hwnd      = hwnd;
		surface_create_info.hinstance = GetModuleHandle(nullptr);
		vk::SurfaceKHR vulkan_surface{p_ctx->vulkanInstance.createWin32SurfaceKHR(surface_create_info)};

		Surface surface{};
		surface.surface = vulkan_surface;
		surface.hwnd    = hwnd;

		return p_ctx->surfaces.emplace(surface);
	}

	auto surface_destroy(GPUContext *p_ctx, SurfaceHandle p_surface) -> void
	{
		p_ctx->surfaces.destroy(p_surface);
	}

	auto semaphore_create(GPUContext *p_ctx, uint64 p_initial_value) -> SemaphoreHandle
	{
		vk::SemaphoreTypeCreateInfo timeline_semaphore_create_info{};
		timeline_semaphore_create_info.initialValue  = p_initial_value;
		timeline_semaphore_create_info.semaphoreType = vk::SemaphoreType::eTimeline;

		vk::SemaphoreCreateInfo create_info{};
		create_info.pNext = &timeline_semaphore_create_info;

		vk::Semaphore semaphore{p_ctx->logicalDevice.createSemaphore(create_info)};

		return p_ctx->semaphores.emplace(Semaphore{semaphore});
	}

	auto semaphore_destroy(GPUContext *p_ctx, SemaphoreHandle p_semaphore) -> void
	{
		p_ctx->semaphores.destroy(p_semaphore);
	}

	auto semaphore_wait_n(GPUContext *p_ctx, InitialiserList<const SemaphoreHandle> p_semaphores, InitialiserList<const uint64> p_values) -> void
	{
		TST_ASSERT(p_semaphores.size() == p_values.size());

		std::vector<vk::Semaphore> semaphores(p_semaphores.size());
		for (uint64 i{0u}; i < p_semaphores.size(); ++i)
			semaphores[i] = p_ctx->semaphores[p_semaphores[i]].semaphore;

		vk::SemaphoreWaitInfo wait_info{};
		wait_info.setSemaphores(semaphores);
		wait_info.setValues(p_values);

		vk::Result res{p_ctx->logicalDevice.waitSemaphores(wait_info, INFINITE)};
		if (res != vk::Result::eSuccess)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to wait on semaphores");
		}
	}

	auto semaphore_get_value(GPUContext *p_ctx, SemaphoreHandle p_semaphore) -> uint64
	{
		uint64     value{0u};
		vk::Result res{p_ctx->logicalDevice.getSemaphoreCounterValue(p_ctx->semaphores[p_semaphore].semaphore, &value)};
		TST_ASSERT_MSG(res == vk::Result::eSuccess, "Failed to query timeline semaphore");
		return value;
	}

	auto texture_create(GPUContext *p_ctx, const TextureDesc &p_desc) -> TextureHandle
	{
		vk::ImageType        image_type{vk::ImageType::e2D};
		vk::ImageViewType    image_view_type{vk::ImageViewType::e2D};
		vk::ImageCreateFlags image_create_flags{0u};

		switch (p_desc.type)
		{
			case ETextureType::e1D:
			{
				image_type      = vk::ImageType::e1D;
				image_view_type = vk::ImageViewType::e1D;
				break;
			}
			case ETextureType::e2D:
			{
				image_type      = vk::ImageType::e2D;
				image_view_type = vk::ImageViewType::e2D;
				break;
			}
			case ETextureType::e3D:
			{
				image_type      = vk::ImageType::e3D;
				image_view_type = vk::ImageViewType::e3D;
				break;
			}
			case ETextureType::eCube:
			{
				image_type         = vk::ImageType::e2D;
				image_view_type    = vk::ImageViewType::eCube;
				image_create_flags |= vk::ImageCreateFlagBits::eCubeCompatible;
				break;
			}
		}

		vk::Format image_format{getVulkanFormat(p_desc.format)};

		vk::ImageUsageFlags image_usage_flags{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eTransferSrc) ? vk::ImageUsageFlagBits::eTransferSrc : vk::ImageUsageFlagBits{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eTransferDst) ? vk::ImageUsageFlagBits::eTransferDst : vk::ImageUsageFlagBits{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eSampled) ? vk::ImageUsageFlagBits::eSampled : vk::ImageUsageFlagBits{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eStorage) ? vk::ImageUsageFlagBits::eStorage : vk::ImageUsageFlagBits{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eColourAttachment) ? vk::ImageUsageFlagBits::eColorAttachment : vk::ImageUsageFlagBits{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eDepthStencilAttachment)
								 ? vk::ImageUsageFlagBits::eDepthStencilAttachment
								 : vk::ImageUsageFlagBits{0u};
		image_usage_flags |= (p_desc.usage & ETextureUsageFlagBits::eTransient) ? vk::ImageUsageFlagBits::eTransientAttachment : vk::ImageUsageFlagBits{0u};

		vk::SampleCountFlagBits sample_count{vk::SampleCountFlagBits::e1};
		switch (p_desc.sampleCount)
		{
			case ESampleCount::e1: sample_count = vk::SampleCountFlagBits::e1;
				break;
			case ESampleCount::e2: sample_count = vk::SampleCountFlagBits::e2;
				break;
			case ESampleCount::e4: sample_count = vk::SampleCountFlagBits::e4;
				break;
			case ESampleCount::e8: sample_count = vk::SampleCountFlagBits::e8;
				break;
			case ESampleCount::e16: sample_count = vk::SampleCountFlagBits::e16;
				break;
			case ESampleCount::e32: sample_count = vk::SampleCountFlagBits::e32;
				break;
			case ESampleCount::e64: sample_count = vk::SampleCountFlagBits::e64;
				break;
		}

		vk::ImageCreateInfo image_create_info{};
		image_create_info.flags         = image_create_flags;
		image_create_info.imageType     = image_type;
		image_create_info.format        = image_format;
		image_create_info.extent        = vk::Extent3D{p_desc.extent.x, p_desc.extent.y, p_desc.extent.z};
		image_create_info.mipLevels     = p_desc.mipCount;
		image_create_info.arrayLayers   = p_desc.layerCount;
		image_create_info.samples       = sample_count;
		image_create_info.tiling        = vk::ImageTiling::eOptimal;
		image_create_info.usage         = image_usage_flags;
		image_create_info.initialLayout = vk::ImageLayout::eUndefined;

		std::array<uint32, 2u> queue_family_indices{p_ctx->queueFamilyIndices.graphics, p_ctx->queueFamilyIndices.transfer}; // Again, I am not doin allat
		if (queue_family_indices[0u] != queue_family_indices[1u])
		{
			image_create_info.sharingMode           = vk::SharingMode::eConcurrent;
			image_create_info.queueFamilyIndexCount = queue_family_indices.size();
			image_create_info.pQueueFamilyIndices   = queue_family_indices.data();
		}
		else
		{
			image_create_info.sharingMode = vk::SharingMode::eExclusive;
		}

		vk::Image     image{nullptr};
		vk::ImageView image_view{nullptr};
		VmaAllocation allocation{nullptr};

		VmaAllocationCreateInfo allocation_create_info{};
		allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

		vmaCreateImage(p_ctx->allocator, reinterpret_cast<const VkImageCreateInfo *>(&image_create_info), &allocation_create_info, reinterpret_cast<VkImage *>(&image),
					   &allocation, nullptr);

		const bool is_render_target{p_desc.usage & ETextureUsageFlagBits::eColourAttachment || p_desc.usage & ETextureUsageFlagBits::eDepthStencilAttachment};
		if (is_render_target)
		{
			vk::ImageAspectFlags image_aspect_mask{getImageAspectMask(p_desc.format)};

			vk::ImageViewCreateInfo image_view_create_info{};
			image_view_create_info.image      = image;
			image_view_create_info.viewType   = image_view_type;
			image_view_create_info.format     = image_format;
			image_view_create_info.components = vk::ComponentMapping{
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity
			};
			image_view_create_info.subresourceRange = vk::ImageSubresourceRange{image_aspect_mask, 0u, p_desc.mipCount, 0u, p_desc.layerCount};

			image_view = p_ctx->logicalDevice.createImageView(image_view_create_info);
		}

		TextureHandle texture_handle{p_ctx->textures.emplace(Texture{image, image_view, allocation, p_desc, false})};

		mtx_lock(&p_ctx->undefinedTexturesMutex);
		p_ctx->undefinedTextures.insert(texture_handle);
		mtx_unlock(&p_ctx->undefinedTexturesMutex);

		return texture_handle;
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

	auto sampler_create(GPUContext *p_ctx, const SamplerDesc &p_desc) -> SamplerHandle
	{
		return p_ctx->samplers.emplace(Sampler{p_desc});
	}

	auto sampler_destroy(GPUContext *p_ctx, SamplerHandle p_sampler) -> void
	{
		p_ctx->samplers.destroy(p_sampler);
	}

	auto createSwapchainObjects(GPUContext *p_ctx, Swapchain &p_swapchain, math::UInt2 p_extent, vk::SwapchainKHR p_old_swapchain = nullptr) -> bool
	{
		vk::Extent2D swapchain_extent{};

		auto &     surface{p_ctx->surfaces[p_swapchain.surface]};
		const auto surface_caps{p_ctx->physicalDevice.getSurfaceCapabilitiesKHR(surface.surface)};

		const auto available_formats{p_ctx->physicalDevice.getSurfaceFormatsKHR(surface.surface)};
		TST_ASSERT(!available_formats.empty());
		// According to my expert research, the most aesthetically pleasing image format is RGBA in the SRGB colour space.
		// If for some reason, your GPU does not support that, then just fall back to the first available format.
		const auto format_it = std::ranges::find_if(available_formats, [](const auto &format)
		{
			return format.format == vk::Format::eR8G8B8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
		vk::SurfaceFormatKHR surface_format{format_it == available_formats.end() ? available_formats[0] : *format_it};

		// Ideally, we want the min image count to be at least 3. However, if your GPU is bad, it might not be able to handle that many images.
		// So if 3 is greater than the max image count, we fall back to the max image count as the min image count... I don't know if that made sense...
		uint32 min_image_count{std::max(3u, surface_caps.minImageCount)};

		// Apparently, if the maxImageCount == 0, then there is no maximum (unlimited).
		if ((surface_caps.maxImageCount > 0) && (surface_caps.maxImageCount < min_image_count))
			min_image_count = surface_caps.maxImageCount;

		auto available_present_modes{p_ctx->physicalDevice.getSurfacePresentModesKHR(surface.surface)};
		// The ideal present mode would be mailbox because it is the fastest.
		// However, not every device supports it. But Fifo is guaranteed to be supported, so that is the fallback option
		TST_ASSERT(!available_present_modes.empty());
		vk::PresentModeKHR present_mode{
			std::ranges::any_of(available_present_modes, [](const auto &present_mode)
			{
				return present_mode == vk::PresentModeKHR::eMailbox;
			})
				? vk::PresentModeKHR::eMailbox
				: vk::PresentModeKHR::eFifo
		};

		p_swapchain.surfaceFormat = surface_format;
		p_swapchain.minImageCount = min_image_count;
		p_swapchain.presentMode   = present_mode;

		// If the current extent is UINT32_MAX, it means that we can choose our own custom extent
		if (surface_caps.currentExtent.width != UINT32_MAX)
			swapchain_extent = vk::Extent2D{surface_caps.currentExtent.width, surface_caps.currentExtent.height};

		// But we still have to make sure that we clamp the extent between the min and max.
		// I think this probably has to do with certain displays (Apple Retina) having a very high pixel density (DPI).
		swapchain_extent = vk::Extent2D{
			std::clamp<uint32>(p_extent.x, surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width),
			std::clamp<uint32>(p_extent.y, surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height)
		};

		if (swapchain_extent.width == 0u || swapchain_extent.height == 0u)
			return false;

		vk::SwapchainCreateInfoKHR swapchain_create_info{};
		swapchain_create_info.surface          = surface.surface;
		swapchain_create_info.minImageCount    = p_swapchain.minImageCount;
		swapchain_create_info.imageFormat      = p_swapchain.surfaceFormat.format;
		swapchain_create_info.imageColorSpace  = p_swapchain.surfaceFormat.colorSpace;
		swapchain_create_info.imageExtent      = swapchain_extent;
		swapchain_create_info.imageArrayLayers = 1u;
		swapchain_create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
		swapchain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
		swapchain_create_info.preTransform     = surface_caps.currentTransform;
		swapchain_create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchain_create_info.presentMode      = p_swapchain.presentMode;
		swapchain_create_info.clipped          = true;
		swapchain_create_info.oldSwapchain     = p_old_swapchain;

		if (p_ctx->logicalDevice.createSwapchainKHR(&swapchain_create_info, nullptr, &p_swapchain.swapchain) != vk::Result::eSuccess)
		{
			p_swapchain.swapchain = nullptr;
			return false;
		}

		p_swapchain.extent = {swapchain_extent.width, swapchain_extent.height};

		TextureDesc attachment_desc{};
		attachment_desc.type        = ETextureType::e2D;
		attachment_desc.extent      = {p_swapchain.extent.x, p_swapchain.extent.y, 1u};
		attachment_desc.mipCount    = 1u;
		attachment_desc.layerCount  = 1u;
		attachment_desc.sampleCount = ESampleCount::e1;
		attachment_desc.format      = getTstFormat(p_swapchain.surfaceFormat.format);
		attachment_desc.usage       = ETextureUsageFlagBits::eColourAttachment;

		auto images{p_ctx->logicalDevice.getSwapchainImagesKHR(p_swapchain.swapchain)};

		p_swapchain.attachments.resize(images.size());
		for (uint32 i{0u}; i < images.size(); ++i)
		{
			vk::ImageViewCreateInfo image_view_create_info{};
			image_view_create_info.image      = images[i];
			image_view_create_info.viewType   = vk::ImageViewType::e2D;
			image_view_create_info.format     = p_swapchain.surfaceFormat.format;
			image_view_create_info.components = vk::ComponentMapping{
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity
			};
			image_view_create_info.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u};

			vk::ImageView image_view{p_ctx->logicalDevice.createImageView(image_view_create_info)};

			// I don't know why I am providing this, as there is no reason to do so. But I guess now you can query the swapchain's texture desc?...
			TextureDesc fake_desc{
				math::UInt3{p_swapchain.extent.x, p_swapchain.extent.y, 1u},
				1u,
				1u,
				ETextureType::e2D,
				ESampleCount::e1,
				getTstFormat(p_swapchain.surfaceFormat.format),
				ETextureUsageFlagBits::eNone
			};
			p_swapchain.attachments[i] = p_ctx->textures.emplace(Texture{images[i], image_view, nullptr, fake_desc, true});
		}

		p_swapchain.renderFinishedSemaphores.resize(images.size());
		for (auto &semaphore: p_swapchain.renderFinishedSemaphores)
			semaphore = p_ctx->logicalDevice.createSemaphore({});

		return true;
	}

	auto destroySwapchainObjects(GPUContext *p_ctx, Swapchain &p_swapchain) -> void
	{
		for (auto &attachment: p_swapchain.attachments)
			p_ctx->textures.destroy(attachment);
		p_swapchain.attachments.clear();

		for (auto &semaphore: p_swapchain.renderFinishedSemaphores)
			p_ctx->logicalDevice.destroySemaphore(semaphore);
		p_swapchain.renderFinishedSemaphores.clear();
	}

	auto swapchain_create(GPUContext *p_ctx, SurfaceHandle p_surface, math::UInt2 p_initial_extent) -> SwapchainHandle
	{
		Swapchain swapchain{};
		swapchain.surface = p_surface;
		swapchain.extent  = p_initial_extent;

		swapchain.imageAvailableSemaphores.resize(p_ctx->maxConcurrentSwapchainWorkloads);
		for (auto &semaphore: swapchain.imageAvailableSemaphores)
			semaphore = p_ctx->logicalDevice.createSemaphore({});

		createSwapchainObjects(p_ctx, swapchain, p_initial_extent);

		return p_ctx->swapchains.emplace(std::move(swapchain));
	}

	auto swapchain_resize(GPUContext *p_ctx, SwapchainHandle p_swapchain, math::UInt2 p_extent) -> bool
	{
		p_ctx->logicalDevice.waitIdle();

		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};

		vk::SwapchainKHR old_swapchain{swapchain.swapchain};

		destroySwapchainObjects(p_ctx, swapchain);
		if (!createSwapchainObjects(p_ctx, swapchain, p_extent, old_swapchain)) // You can handle ts on your own
			return false;

		if (old_swapchain)
			p_ctx->logicalDevice.destroySwapchainKHR(old_swapchain);

		return true;
	}

	auto swapchain_destroy(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> void
	{
		p_ctx->swapchains.destroy(p_swapchain);
	}

	auto swapchain_acquire_next_image(GPUContext *p_ctx, SwapchainHandle p_swapchain) -> TextureHandle
	{
		Swapchain &swapchain{p_ctx->swapchains[p_swapchain]};
		if (!swapchain.swapchain)
			return nullptr;

		swapchain.acquisitonIndex = (swapchain.acquisitonIndex + 1u) % swapchain.imageAvailableSemaphores.size();

		auto [res, idx]{p_ctx->logicalDevice.acquireNextImageKHR(swapchain.swapchain, INFINITE, swapchain.imageAvailableSemaphores[swapchain.acquisitonIndex], nullptr)};
		swapchain.imageIndex = idx;
		if (res != vk::Result::eSuccess)
			return nullptr;

		return swapchain.attachments[swapchain.imageIndex];
	}

	auto swapchain_insert_pre_render_resource_barrier(GPUContext *p_ctx, CommandListHandle p_cmd_list, TextureHandle p_attachment_texture) -> void
	{
		CommandList &cmd{p_ctx->commandLists[p_cmd_list]};
		TST_ASSERT(cmd.open);

		Texture &attachment{p_ctx->textures[p_attachment_texture]};

		vk::ImageMemoryBarrier2 undefined_to_general{};
		undefined_to_general.image               = attachment.image;
		undefined_to_general.srcAccessMask       = vk::AccessFlagBits2::eNone;
		undefined_to_general.dstAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite;
		undefined_to_general.srcStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		undefined_to_general.dstStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		undefined_to_general.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
		undefined_to_general.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
		undefined_to_general.oldLayout           = vk::ImageLayout::eUndefined;
		undefined_to_general.newLayout           = vk::ImageLayout::eGeneral;
		undefined_to_general.subresourceRange    = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u};

		vk::DependencyInfo dependency_info{};
		dependency_info.setImageMemoryBarriers(undefined_to_general);
		cmd.cmd.pipelineBarrier2(dependency_info);
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

		vk::ImageMemoryBarrier2 general_to_present_src{};
		general_to_present_src.image               = colour_attachment.image;
		general_to_present_src.srcAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite;
		general_to_present_src.dstAccessMask       = vk::AccessFlagBits2::eNone;
		general_to_present_src.srcStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		general_to_present_src.dstStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		general_to_present_src.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
		general_to_present_src.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
		general_to_present_src.oldLayout           = vk::ImageLayout::eGeneral;
		general_to_present_src.newLayout           = vk::ImageLayout::ePresentSrcKHR;
		general_to_present_src.subresourceRange    = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u};

		vk::DependencyInfo dependency_info{};
		dependency_info.setImageMemoryBarriers(general_to_present_src);

		cmd.cmd.pipelineBarrier2(dependency_info);

		cmd_list_close(p_ctx, p_cmd_list);

		std::vector<vk::SemaphoreSubmitInfo> raw_waits;
		raw_waits.reserve(1u + p_wait_semaphore_infos.size());
		raw_waits.emplace_back(swapchain.imageAvailableSemaphores[swapchain.acquisitonIndex]);
		for (const SemaphoreSubmitInfo &wait: p_wait_semaphore_infos)
			raw_waits.emplace_back(p_ctx->semaphores[wait.semaphore].semaphore, wait.value);

		submit(p_ctx, EQueueType::eGraphics, p_cmd_list, raw_waits, {
				   vk::SemaphoreSubmitInfo{swapchain.renderFinishedSemaphores[swapchain.imageIndex]},
				   vk::SemaphoreSubmitInfo{p_ctx->semaphores[p_signal_semaphore_info.semaphore].semaphore, p_signal_semaphore_info.value}
			   });

		vk::PresentInfoKHR present_info{};
		present_info.setSwapchains(swapchain.swapchain);
		present_info.setImageIndices(swapchain.imageIndex);
		present_info.setWaitSemaphores(swapchain.renderFinishedSemaphores[swapchain.imageIndex]);

		// Avoids vulkan hpp exceptions
		const vk::Result res{
			FunctionDispatcher::get().vkQueuePresentKHR(p_ctx->queues[static_cast<uint32>(EQueueType::eGraphics)],
														reinterpret_cast<const VkPresentInfoKHR *>(&present_info))
		};

		if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR)
			return false;
		else if (res != vk::Result::eSuccess)
			TST_PERMA_ASSERT_MSG(false, "Failed to present swapchain image!");

		return true;
	}

	auto shader_create(GPUContext *p_ctx, const ShaderDesc &p_desc) -> ShaderHandle
	{
		vk::ShaderStageFlagBits shader_stage{static_cast<vk::ShaderStageFlagBits>(static_cast<uint32>(getVulkanShaderStages(p_desc.stage)))};
		vk::ShaderStageFlags    next_stage{getVulkanShaderStages(p_desc.nextStage)};

		vk::ShaderCreateInfoEXT shader_create_info{};
		shader_create_info.flags = vk::ShaderCreateFlagBitsEXT::eDescriptorHeap | ((!p_desc.task && shader_stage & vk::ShaderStageFlagBits::eMeshEXT)
																					   ? vk::ShaderCreateFlagBitsEXT::eNoTaskShader
																					   : vk::ShaderCreateFlagBitsEXT{0u});
		shader_create_info.stage     = shader_stage;
		shader_create_info.nextStage = next_stage;
		shader_create_info.codeType  = vk::ShaderCodeTypeEXT::eSpirv;
		shader_create_info.codeSize  = p_desc.codeSizeWords * sizeof(uint32);
		shader_create_info.pCode     = p_desc.code;
		shader_create_info.pName     = p_desc.entryPointName;

		auto [res, shader]{p_ctx->logicalDevice.createShaderEXT(shader_create_info, nullptr, FunctionDispatcher::get())};
		if (res != vk::Result::eSuccess)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to create shader!");
		}

		return p_ctx->shaders.emplace(Shader{shader, shader_stage, next_stage});
	}

	auto shader_destroy(GPUContext *p_ctx, ShaderHandle p_shader) -> void
	{
		p_ctx->shaders.destroy(p_shader);
	}
}
