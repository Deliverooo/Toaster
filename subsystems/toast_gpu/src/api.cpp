#include "toast_gpu/api.hpp"

#include "toast_lib/pool.hpp"
#include "toast_lib/freelist_allocator.hpp"

#include <unordered_set>
#include <algorithm>

#define VK_USE_PLATFORM_WIN32_KHR
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
		UniquePtr<std::mutex> bufferSlotMutex{nullptr};
		UniquePtr<std::mutex> imageSlotMutex{nullptr};

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
		UniquePtr<std::mutex> samplerSlotMutex{nullptr};

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

		DeviceAddress address{0u};

		void *mapped{nullptr};

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
		tsm::uint2       extent{0u};

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

	struct APIImpl
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

		QueueFamilyIndices queueFamilyIndices{};

		std::array<vk::Queue, 3u> queues;

		#pragma endregion

		#pragma region allocator

		VmaAllocator allocator{nullptr};

		#pragma endregion

		#pragma region descriptor heaps

		Pool<ResourceDescriptorHeap> resourceHeaps;
		Pool<SamplerDescriptorHeap>  samplerHeaps;

		#pragma endregion

		#pragma region command lists

		Pool<CommandList> commandLists;

		std::array<std::vector<CommandListHandle>, 3u> freeCommandLists;   // Free command lists per queue type
		std::array<std::vector<CommandListHandle>, 3u> freeSecondaryLists; // Free command lists per queue type

		std::array<vk::CommandPool, 3u> transientPools; // Mostly used for allocating command buffers for image layout transitions

		#pragma endregion

		TST_REGISTER_RESOURCE_POOL(Semaphore, semaphores);
		TST_REGISTER_RESOURCE_POOL(Buffer, buffers);
		TST_REGISTER_RESOURCE_POOL(Texture, textures);

		std::unordered_set<TextureHandle> undefinedTextures; // All the textures that are in the undefined format and are awaiting a layout transition
		std::mutex                        undefinedTexturesMutex;

		#pragma region samplers

		Pool<Sampler> samplers;

		#pragma endregion

		#pragma region swapchain

		Pool<Surface>   surfaces;
		Pool<Swapchain> swapchains;

		#pragma endregion

		TST_REGISTER_RESOURCE_POOL(Shader, shaders);
	};

	static APIImpl *g_impl{nullptr};

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
	}

	auto initGPUContext(const GPUContextDesc &p_desc) -> void
	{
		if (g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "API has already been initialised!");
			return;
		}

		g_impl = new APIImpl{};

		g_impl->usingSwapchain                  = p_desc.usingSwapchain;
		g_impl->maxConcurrentSwapchainWorkloads = p_desc.maxConcurrentSwapchainWorkloads;

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

			g_impl->vulkanInstance = vk::createInstance(instance_create_info);

			FunctionDispatcher::initInstanceFunctions(g_impl->vulkanInstance);
		}

		#pragma endregion

		ExtensionSet required_device_extensions{
			vk::EXTDescriptorHeapExtensionName,
			vk::EXTShaderObjectExtensionName,
			vk::KHRMaintenance1ExtensionName,
			vk::KHRShaderUntypedPointersExtensionName,
			vk::KHRMaintenance9ExtensionName,
			vk::KHRUnifiedImageLayoutsExtensionName
		};
		if (p_desc.usingSwapchain)
			required_device_extensions.emplace(vk::KHRSwapchainExtensionName);

		#pragma region physical device
		{
			auto physical_devices{g_impl->vulkanInstance.enumeratePhysicalDevices()};
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
							vk::PhysicalDeviceDescriptorHeapFeaturesEXT>()
					};

					const bool supports_required_features{
						features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy && features.get<vk::PhysicalDeviceFeatures2>().features.sampleRateShading
						&& features.get<vk::PhysicalDeviceFeatures2>().features.fillModeNonSolid && features.get<vk::PhysicalDeviceVulkan12Features>().timelineSemaphore
						&& features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
						features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState && features.get<
							vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR>().unifiedImageLayouts && features.get<vk::PhysicalDeviceDescriptorHeapFeaturesEXT>().
						descriptorHeap
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

			g_impl->physicalDevice = *device_it;

			g_impl->deviceProperties.pNext = &g_impl->descriptorHeapProperties;
			g_impl->physicalDevice.getProperties2(&g_impl->deviceProperties);
		}

		#pragma endregion

		#pragma region logical device

		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceVulkan14Features, vk::PhysicalDeviceShaderObjectFeaturesEXT, vk::PhysicalDeviceDescriptorHeapFeaturesEXT,
			vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR, vk::PhysicalDeviceMaintenance9FeaturesKHR, vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR> feature_chain
				{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy                        = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.sampleRateShading                        = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.fillModeNonSolid                         = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.fragmentStoresAndAtomics                 = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.shaderInt16                              = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.shaderInt64                              = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.vertexPipelineStoresAndAtomics           = true;
		feature_chain.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect                        = true;
		feature_chain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters                       = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().scalarBlockLayout                          = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().timelineSemaphore                          = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress                        = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().runtimeDescriptorArray                     = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderInt8                                 = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().storagePushConstant8                       = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().uniformAndStorageBuffer8BitAccess          = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().storageBuffer8BitAccess                    = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderStorageBufferArrayNonUniformIndexing = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderUniformBufferArrayNonUniformIndexing = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderSampledImageArrayNonUniformIndexing  = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderUniformBufferArrayNonUniformIndexing = true;
		feature_chain.get<vk::PhysicalDeviceVulkan12Features>().shaderStorageImageArrayNonUniformIndexing  = true;
		feature_chain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering                           = true;
		feature_chain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2                           = true;
		feature_chain.get<vk::PhysicalDeviceVulkan13Features>().maintenance4                               = true;
		feature_chain.get<vk::PhysicalDeviceVulkan14Features>().indexTypeUint8                             = true;
		feature_chain.get<vk::PhysicalDeviceVulkan14Features>().maintenance6                               = true;
		feature_chain.get<vk::PhysicalDeviceShaderObjectFeaturesEXT>().shaderObject                        = true;
		feature_chain.get<vk::PhysicalDeviceDescriptorHeapFeaturesEXT>().descriptorHeap                    = true;
		feature_chain.get<vk::PhysicalDeviceShaderUntypedPointersFeaturesKHR>().shaderUntypedPointers      = true;
		feature_chain.get<vk::PhysicalDeviceMaintenance9FeaturesKHR>().maintenance9                        = true;
		feature_chain.get<vk::PhysicalDeviceUnifiedImageLayoutsFeaturesKHR>().unifiedImageLayouts          = true;

		g_impl->queueFamilyIndices = selectQueueFamilyIndices(g_impl->physicalDevice, p_desc.usingSwapchain);

		if (g_impl->queueFamilyIndices.graphics == UINT32_MAX)
			TST_PERMA_ASSERT_MSG(false, "Failed to find a graphics queue family");

		if (g_impl->queueFamilyIndices.transfer == UINT32_MAX)
			g_impl->queueFamilyIndices.transfer = g_impl->queueFamilyIndices.graphics;

		constexpr float32 default_queue_priority{1.0f};

		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		queue_create_infos.emplace_back(vk::DeviceQueueCreateInfo{{}, g_impl->queueFamilyIndices.graphics, 1u, &default_queue_priority});
		if (g_impl->queueFamilyIndices.transfer != g_impl->queueFamilyIndices.graphics)
			queue_create_infos.emplace_back(vk::DeviceQueueCreateInfo{{}, g_impl->queueFamilyIndices.transfer, 1u, &default_queue_priority});

		const auto enabled_extensions_vec{required_device_extensions | std::ranges::to<std::vector>()};

		vk::DeviceCreateInfo logical_device_create_info{};
		logical_device_create_info.queueCreateInfoCount    = queue_create_infos.size();
		logical_device_create_info.pQueueCreateInfos       = queue_create_infos.data();
		logical_device_create_info.enabledExtensionCount   = enabled_extensions_vec.size();
		logical_device_create_info.ppEnabledExtensionNames = enabled_extensions_vec.data();
		logical_device_create_info.pNext                   = feature_chain.get<vk::PhysicalDeviceFeatures2>();

		g_impl->logicalDevice = g_impl->physicalDevice.createDevice(logical_device_create_info);

		vk::Queue graphics_queue{g_impl->logicalDevice.getQueue(g_impl->queueFamilyIndices.graphics, 0u)};
		vk::Queue compute_queue{graphics_queue}; // TODO: More queues
		vk::Queue transfer_queue{
			g_impl->logicalDevice.getQueue(g_impl->queueFamilyIndices.transfer, (g_impl->queueFamilyIndices.transfer == g_impl->queueFamilyIndices.graphics) ? 1u : 0u)
		};

		g_impl->queues[0] = graphics_queue;
		g_impl->queues[1] = compute_queue;
		g_impl->queues[2] = transfer_queue;

		for (uint32 queue_index{0u}; queue_index < 3u; ++queue_index)
		{
			vk::CommandPoolCreateInfo command_pool_create_info{};
			command_pool_create_info.queueFamilyIndex = g_impl->queueFamilyIndices.get((EQueueType) queue_index);
			command_pool_create_info.flags            = vk::CommandPoolCreateFlagBits::eTransient;
			g_impl->transientPools[queue_index]       = g_impl->logicalDevice.createCommandPool(command_pool_create_info);
		}

		FunctionDispatcher::initDeviceFunctions(g_impl->logicalDevice);

		#pragma endregion

		#pragma region allocator

		VmaAllocatorCreateInfo allocator_create_info{};
		allocator_create_info.instance       = g_impl->vulkanInstance;
		allocator_create_info.physicalDevice = g_impl->physicalDevice;
		allocator_create_info.device         = g_impl->logicalDevice;
		allocator_create_info.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocator_create_info, &g_impl->allocator);

		#pragma endregion

		#pragma region destroy callbacks

		g_impl->commandLists.setDestructorFn([](CommandList *p_data, void *) mutable noexcept-> void
		{
			g_impl->logicalDevice.destroyCommandPool(p_data->pool);
		});

		g_impl->resourceHeaps.setDestructorFn([](ResourceDescriptorHeap *p_data, void *) mutable noexcept-> void
		{
			vmaUnmapMemory(g_impl->allocator, p_data->heapAllocation);
			vmaDestroyBuffer(g_impl->allocator, p_data->heapBuffer, p_data->heapAllocation);
		});

		g_impl->samplerHeaps.setDestructorFn([](SamplerDescriptorHeap *p_data, void *) mutable noexcept-> void
		{
			vmaUnmapMemory(g_impl->allocator, p_data->heapAllocation);
			vmaDestroyBuffer(g_impl->allocator, p_data->heapBuffer, p_data->heapAllocation);
		});

		g_impl->semaphores.setDestructorFn([](Semaphore *p_data, void *) mutable noexcept-> void
		{
			g_impl->logicalDevice.destroySemaphore(p_data->semaphore);
		});

		g_impl->buffers.setDestructorFn([](Buffer *p_data, void *) mutable noexcept-> void
		{
			vmaDestroyBuffer(g_impl->allocator, p_data->buffer, p_data->allocation);
		});

		g_impl->textures.setDestructorFn([](Texture *p_data, void *) mutable noexcept-> void
		{
			if (p_data->imageView)
				g_impl->logicalDevice.destroyImageView(p_data->imageView);

			if (!p_data->isSwapchainImage)
				vmaDestroyImage(g_impl->allocator, p_data->image, p_data->allocation);

			p_data->image      = nullptr;
			p_data->allocation = nullptr;
			p_data->imageView  = nullptr;
		});

		g_impl->surfaces.setDestructorFn([](Surface *p_data, void *) mutable noexcept-> void
		{
			g_impl->vulkanInstance.destroySurfaceKHR(p_data->surface);
		});

		g_impl->swapchains.setDestructorFn([](Swapchain *p_data, void *) mutable noexcept-> void
		{
			for (auto &attachment: p_data->attachments)
				g_impl->textures.destroy(attachment);

			for (auto &semaphore: p_data->imageAvailableSemaphores)
				g_impl->logicalDevice.destroySemaphore(semaphore);

			for (auto &semaphore: p_data->renderFinishedSemaphores)
				g_impl->logicalDevice.destroySemaphore(semaphore);

			g_impl->logicalDevice.destroySwapchainKHR(p_data->swapchain);
		});

		g_impl->shaders.setDestructorFn([](Shader *p_data, void *) mutable noexcept-> void
		{
			g_impl->logicalDevice.destroyShaderEXT(p_data->shader, nullptr, FunctionDispatcher::get());
		});

		#pragma endregion
	}

	auto shutdownGPUContext() -> void
	{
		if (!g_impl)
		{
			TST_PERMA_ASSERT_MSG(false, "API has not been initialised!");
			return;
		}

		g_impl->logicalDevice.waitIdle();

		g_impl->resourceHeaps.clear();
		g_impl->samplerHeaps.clear();
		g_impl->textures.clear();
		g_impl->samplers.clear();
		g_impl->buffers.clear();
		g_impl->shaders.clear();
		g_impl->surfaces.clear();
		g_impl->semaphores.clear();
		g_impl->commandLists.clear();

		for (uint32 queue_index{0u}; queue_index < 3u; ++queue_index)
			g_impl->logicalDevice.destroyCommandPool(g_impl->transientPools[queue_index]);

		vmaDestroyAllocator(g_impl->allocator);
		g_impl->logicalDevice.destroy();
		g_impl->vulkanInstance.destroy();

		delete g_impl;
		g_impl = nullptr;
	}

	auto waitIdle() -> void
	{
		g_impl->logicalDevice.waitIdle();
	}

	auto waitQueueIdle(EQueueType p_queue_type) -> void
	{
		vk::Queue queue{g_impl->queues[static_cast<uint32>(p_queue_type)]};
		queue.waitIdle();
	}

	auto getPendingImageMemoryBarriers(EQueueType p_queue_type) -> vk::CommandBuffer
	{
		std::scoped_lock<std::mutex> lock{g_impl->undefinedTexturesMutex};

		if (g_impl->undefinedTextures.empty() || p_queue_type == EQueueType::eTransfer) // Transfer queues cannot perform layout transitions
			return nullptr;

		std::vector<vk::ImageMemoryBarrier2> image_memory_barriers(g_impl->undefinedTextures.size());

		uint32 i{0u};
		for (auto undef_tex: g_impl->undefinedTextures)
		{
			const Texture &texture{g_impl->textures[undef_tex]};
			TST_PERMA_ASSERT(g_impl->textures.isValid(undef_tex));

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

		vk::DependencyInfo dependency_info{};
		dependency_info.setImageMemoryBarriers(image_memory_barriers);

		vk::CommandPool pool{g_impl->transientPools[static_cast<uint32>(p_queue_type)]};

		vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info.commandPool        = pool;
		command_buffer_alloc_info.commandBufferCount = 1u;
		command_buffer_alloc_info.level              = vk::CommandBufferLevel::ePrimary;
		vk::CommandBuffer cmd{g_impl->logicalDevice.allocateCommandBuffers(command_buffer_alloc_info).front()};

		g_impl->logicalDevice.setDebugUtilsObjectNameEXT(cmd, "Image memory barrier cmd", FunctionDispatcher::get());

		cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		cmd.pipelineBarrier2(dependency_info);
		cmd.end();

		return cmd;
	}

	auto getOrCreateCommandList(EQueueType p_queue_type, bool p_secondary) -> CommandListHandle
	{
		auto &free_lists{p_secondary ? g_impl->freeSecondaryLists[static_cast<uint32>(p_queue_type)] : g_impl->freeCommandLists[static_cast<uint32>(p_queue_type)]};
		if (!free_lists.empty())
		{
			CommandListHandle command_list_handle{free_lists.back()};
			free_lists.pop_back();

			return command_list_handle;
		}
		vk::CommandPoolCreateInfo command_pool_create_info{};
		command_pool_create_info.queueFamilyIndex = g_impl->queueFamilyIndices.get(p_queue_type);
		command_pool_create_info.flags            = vk::CommandPoolCreateFlagBits::eTransient;
		vk::CommandPool pool{g_impl->logicalDevice.createCommandPool(command_pool_create_info)};

		vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info.commandPool        = pool;
		command_buffer_alloc_info.commandBufferCount = 1u;
		command_buffer_alloc_info.level              = p_secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
		vk::CommandBuffer cmd{g_impl->logicalDevice.allocateCommandBuffers(command_buffer_alloc_info).front()};

		g_impl->logicalDevice.setDebugUtilsObjectNameEXT(cmd, p_queue_type == EQueueType::eTransfer ? "Transfer cmd" : "Graphics cmd", FunctionDispatcher::get());

		return g_impl->commandLists.emplace(CommandList{cmd, pool, p_queue_type, false, p_secondary});
	}

	auto openCommandList(CommandListHandle p_command_list, const CommandListInheritanceInfo *p_inheritance_info) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		TST_ASSERT(!cmd.open);
		cmd.open = true;

		vk::CommandBufferInheritanceDescriptorHeapInfoEXT descriptor_heap_inheritance_info{};
		if (cmd.secondary && p_inheritance_info)
		{
			ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_inheritance_info->resourceHeap]};
			SamplerDescriptorHeap & sampler_heap{g_impl->samplerHeaps[p_inheritance_info->samplerHeap]};

			descriptor_heap_inheritance_info.pResourceHeapBindInfo = &resource_heap.bindInfo;
			descriptor_heap_inheritance_info.pSamplerHeapBindInfo  = &sampler_heap.bindInfo;
		}

		vk::CommandBufferInheritanceRenderingInfo rendering_inheritance_info{};
		std::vector<vk::Format>                   colour_attachment_formats;
		if (cmd.secondary && p_inheritance_info)
		{
			colour_attachment_formats = p_inheritance_info->colourAttachmentFormats | std::views::transform([](EFormat format) -> vk::Format
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

	auto closeCommandList(CommandListHandle p_command_list) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		TST_ASSERT(cmd.open);
		cmd.open = false;
		cmd.cmd.end();
	}

	auto freeCommandList(CommandListHandle p_command_list) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		TST_ASSERT(!cmd.open);

		if (cmd.secondary)
			g_impl->freeSecondaryLists[static_cast<uint32>(cmd.queueType)].push_back(p_command_list);
		else
			g_impl->freeCommandLists[static_cast<uint32>(cmd.queueType)].push_back(p_command_list);
	}

	auto resetCommandList(CommandListHandle p_command_list) -> void
	{
		CommandList &command_list{g_impl->commandLists[p_command_list]};

		if (command_list.open)
			command_list.open = false;

		g_impl->logicalDevice.resetCommandPool(command_list.pool);
	}

	// Accepts raw Vulkan semaphores
	static auto submit(EQueueType                                     p_queue_type, InitialiserList<const CommandListHandle> p_command_lists,
					   InitialiserList<const vk::SemaphoreSubmitInfo> p_wait_semaphore_infos,
					   InitialiserList<const vk::SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void
	{
		vk::CommandBuffer image_memory_barrier_cmd{getPendingImageMemoryBarriers(p_queue_type)};

		std::vector<vk::CommandBufferSubmitInfo> command_buffer_submit_infos(p_command_lists.size());
		for (uint64 i{0u}; i < p_command_lists.size(); ++i)
		{
			CommandList &cmd_list{g_impl->commandLists[p_command_lists[i]]};
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

		vk::Queue queue{g_impl->queues[static_cast<uint32>(p_queue_type)]};

		queue.submit2(submit_info);
	}

	auto submit(EQueueType p_queue_type, InitialiserList<const CommandListHandle> p_command_lists, InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos,
				InitialiserList<const SemaphoreSubmitInfo> p_signal_semaphore_infos) -> void
	{
		std::vector<vk::SemaphoreSubmitInfo> wait_infos(p_wait_semaphore_infos.size());
		for (uint64 i{0u}; i < p_wait_semaphore_infos.size(); ++i)
		{
			const auto &src_wait_info{p_wait_semaphore_infos[i]};

			vk::Semaphore semaphore{g_impl->semaphores[src_wait_info.semaphore].semaphore};

			auto &dst_wait_info{wait_infos[i]};
			dst_wait_info.semaphore = semaphore;
			dst_wait_info.value     = src_wait_info.value;
		}

		std::vector<vk::SemaphoreSubmitInfo> signal_infos(p_signal_semaphore_infos.size());
		for (uint64 i{0u}; i < p_signal_semaphore_infos.size(); ++i)
		{
			const auto &src_signal_info{p_signal_semaphore_infos[i]};

			vk::Semaphore semaphore{g_impl->semaphores[src_signal_info.semaphore].semaphore};

			auto &dst_signal_info{signal_infos[i]};
			dst_signal_info.semaphore = semaphore;
			dst_signal_info.value     = src_signal_info.value;
		}

		submit(p_queue_type, p_command_lists, wait_infos, signal_infos);
	}

	auto executeCommandLists(CommandListHandle p_primary_command_list, InitialiserList<const CommandListHandle> p_secondary_command_lists) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_primary_command_list]};
		TST_ASSERT(!cmd.secondary && cmd.open);

		std::vector<vk::CommandBuffer> secondary_lists(p_secondary_command_lists.size());
		for (uint32 i{0u}; i < p_secondary_command_lists.size(); ++i)
		{
			CommandList &secondary{g_impl->commandLists[p_secondary_command_lists[i]]};
			TST_ASSERT(secondary.secondary && !secondary.open);
			secondary_lists[i] = secondary.cmd;
		}

		cmd.cmd.executeCommands(secondary_lists, FunctionDispatcher::get());
	}

	auto copyBuffer(CommandListHandle p_command_list, BufferHandle p_src_buffer, BufferHandle p_dst_buffer, uint64 p_size, uint64 p_src_offset,
					uint64            p_dst_offset) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		Buffer &src_buffer{g_impl->buffers[p_src_buffer]};
		Buffer &dst_buffer{g_impl->buffers[p_dst_buffer]};

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

	auto copyBufferToTexture(CommandListHandle p_command_list, BufferHandle p_src_buffer, TextureHandle p_dst_texture, uint64 p_src_offset, uint32 p_mip_level,
							 uint32            p_base_layer, uint32         p_layer_count, tsm::uint3   p_extent) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		TST_ASSERT(cmd.open);

		Texture *dst_texture{g_impl->textures.tryGet(p_dst_texture)};
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
			std::scoped_lock<std::mutex> lock{g_impl->undefinedTexturesMutex};
			auto                         undefined_it{std::ranges::find(g_impl->undefinedTextures, p_dst_texture)};
			if (undefined_it != g_impl->undefinedTextures.end())
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
				undefined_it = g_impl->undefinedTextures.erase(undefined_it);
			}
		}

		vk::BufferImageCopy2 copy_region{};
		copy_region.bufferOffset      = p_src_offset;
		copy_region.bufferRowLength   = 0u; // TODO: More options for advanced copy operations
		copy_region.bufferImageHeight = 0u;
		copy_region.imageSubresource  = vk::ImageSubresourceLayers{getImageAspectMask(dst_texture->desc.format), p_mip_level, p_base_layer, p_layer_count};
		copy_region.imageOffset       = vk::Offset3D{0u, 0u, 0u};
		copy_region.imageExtent       = vk::Extent3D{p_extent.x, p_extent.y, p_extent.z};

		vk::CopyBufferToImageInfo2 copy_buffer_to_image_info{};
		copy_buffer_to_image_info.setRegions(copy_region);

		Buffer &src_buffer{g_impl->buffers[p_src_buffer]};

		copy_buffer_to_image_info.srcBuffer      = src_buffer.buffer;
		copy_buffer_to_image_info.dstImage       = dst_texture->image;
		copy_buffer_to_image_info.dstImageLayout = vk::ImageLayout::eGeneral; // Thank you VK_KHR_unified_image_layouts :)

		cmd.cmd.copyBufferToImage2(copy_buffer_to_image_info);
	}

	auto generateMipmaps(CommandListHandle p_command_list, TextureHandle p_texture) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		TST_ASSERT(cmd.open);

		Texture *dst_texture{g_impl->textures.tryGet(p_texture)};
		TST_PERMA_ASSERT(dst_texture);
		TST_ASSERT_MSG(dst_texture->desc.mipCount > 1u, "Texture does not contain mip levels to generate");
		TST_ASSERT_MSG(dst_texture->desc.usage & ETextureUsageFlagBits::eTransferSrc, "Texture was not created with the usage of transfer src");
		TST_ASSERT_MSG(dst_texture->desc.usage & ETextureUsageFlagBits::eTransferDst, "Texture was not created with the usage of transfer dst");

		vk::ImageMemoryBarrier2 memory_barrier{};
		memory_barrier.image                           = dst_texture->image;
		memory_barrier.oldLayout                       = vk::ImageLayout::eGeneral;
		memory_barrier.newLayout                       = vk::ImageLayout::eGeneral;
		memory_barrier.srcAccessMask                   = vk::AccessFlagBits2::eTransferWrite;
		memory_barrier.dstAccessMask                   = vk::AccessFlagBits2::eTransferRead;
		memory_barrier.srcQueueFamilyIndex             = vk::QueueFamilyIgnored;
		memory_barrier.dstQueueFamilyIndex             = vk::QueueFamilyIgnored;
		memory_barrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
		memory_barrier.subresourceRange.baseArrayLayer = 0u;
		memory_barrier.subresourceRange.baseMipLevel   = 0u;
		memory_barrier.subresourceRange.layerCount     = 1u;
		memory_barrier.subresourceRange.levelCount     = 1u;

		int32 mip_width{static_cast<int32>(dst_texture->desc.extent.x)};
		int32 mip_height{static_cast<int32>(dst_texture->desc.extent.y)};

		vk::ImageAspectFlags image_aspect{getImageAspectMask(dst_texture->desc.format)};

		for (uint32 i{1u}; i < dst_texture->desc.mipCount; ++i)
		{
			memory_barrier.subresourceRange.baseMipLevel = i - 1;
			memory_barrier.srcAccessMask                 = vk::AccessFlagBits2::eTransferWrite;
			memory_barrier.dstAccessMask                 = vk::AccessFlagBits2::eTransferRead;

			{
				memory_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
				memory_barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;

				vk::DependencyInfo dependency_info{};
				dependency_info.imageMemoryBarrierCount = 1;
				dependency_info.pImageMemoryBarriers    = &memory_barrier;
				cmd.cmd.pipelineBarrier2(dependency_info);
			}

			std::array<vk::Offset3D, 2> src_offsets;
			std::array<vk::Offset3D, 2> dst_offsets;

			src_offsets[0] = vk::Offset3D{0, 0, 0};
			src_offsets[1] = vk::Offset3D{mip_width, mip_height, 1};

			dst_offsets[0] = vk::Offset3D{0, 0, 0};
			dst_offsets[1] = vk::Offset3D{mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};

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

			memory_barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
			memory_barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;

			while (mip_width > 1)
				mip_width /= 2;
			while (mip_height > 1)
				mip_height /= 2;

			{
				memory_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
				memory_barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;

				vk::DependencyInfo dependency_info{};
				dependency_info.imageMemoryBarrierCount = 1;
				dependency_info.pImageMemoryBarriers    = &memory_barrier;
				cmd.cmd.pipelineBarrier2(dependency_info);
			}
		}

		memory_barrier.subresourceRange.baseMipLevel = dst_texture->desc.mipCount - 1u;
		memory_barrier.srcAccessMask                 = vk::AccessFlagBits2::eTransferWrite;
		memory_barrier.dstAccessMask                 = vk::AccessFlagBits2::eShaderRead;

		{
			memory_barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
			memory_barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;

			vk::DependencyInfo dependency_info{};
			dependency_info.imageMemoryBarrierCount = 1;
			dependency_info.pImageMemoryBarriers    = &memory_barrier;
			cmd.cmd.pipelineBarrier2(dependency_info);
		}
	}

	auto beginRendering(CommandListHandle p_command_list, const RenderingInfo &p_rendering_info) -> void
	{
		std::vector<vk::RenderingAttachmentInfo> colour_attachments(p_rendering_info.colourAttachments.size());
		for (uint32 i{0u}; i < p_rendering_info.colourAttachments.size(); ++i)
		{
			auto &src_attachment{p_rendering_info.colourAttachments[i]};
			auto &dst_attachment{colour_attachments[i]};

			const Texture &render_target{g_impl->textures[src_attachment.renderTarget]};
			dst_attachment.imageView   = render_target.imageView;
			dst_attachment.imageLayout = vk::ImageLayout::eGeneral;

			dst_attachment.loadOp     = getLoadOp(src_attachment.usageOp);
			dst_attachment.storeOp    = getStoreOp(src_attachment.usageOp);
			dst_attachment.clearValue = *reinterpret_cast<const vk::ClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (g_impl->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture &resolve_render_target{g_impl->textures[src_attachment.resolveTarget]};
				dst_attachment.resolveImageView   = resolve_render_target.imageView;
				dst_attachment.resolveImageLayout = vk::ImageLayout::eGeneral;
				dst_attachment.resolveMode        = getResolveMode(src_attachment.resolveMode);
			}
		}

		vk::RenderingAttachmentInfo depth_attachment{};
		if (p_rendering_info.depthAttachment.has_value())
		{
			auto &src_attachment{p_rendering_info.depthAttachment.value()};

			const Texture &render_target{g_impl->textures[src_attachment.renderTarget]};
			depth_attachment.imageView   = render_target.imageView;
			depth_attachment.imageLayout = vk::ImageLayout::eGeneral;

			depth_attachment.loadOp     = getLoadOp(src_attachment.usageOp);
			depth_attachment.storeOp    = getStoreOp(src_attachment.usageOp);
			depth_attachment.clearValue = *reinterpret_cast<const vk::ClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (g_impl->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture &resolve_render_target{g_impl->textures[src_attachment.resolveTarget]};
				depth_attachment.resolveImageView   = resolve_render_target.imageView;
				depth_attachment.resolveImageLayout = vk::ImageLayout::eGeneral;
				depth_attachment.resolveMode        = getResolveMode(src_attachment.resolveMode);
			}
		}

		vk::RenderingAttachmentInfo stencil_attachment{};
		if (p_rendering_info.stencilAttachment.has_value())
		{
			auto &         src_attachment{p_rendering_info.stencilAttachment.value()};
			const Texture &render_target{g_impl->textures[src_attachment.renderTarget]};
			stencil_attachment.imageView   = render_target.imageView;
			stencil_attachment.imageLayout = vk::ImageLayout::eGeneral;

			stencil_attachment.loadOp     = getLoadOp(src_attachment.usageOp);
			stencil_attachment.storeOp    = getStoreOp(src_attachment.usageOp);
			stencil_attachment.clearValue = *reinterpret_cast<const vk::ClearValue *>(&src_attachment.clearValue); // They should have the same memory layout

			if (g_impl->textures.isValid(src_attachment.resolveTarget))
			{
				const Texture &resolve_render_target{g_impl->textures[src_attachment.resolveTarget]};
				stencil_attachment.resolveImageView   = resolve_render_target.imageView;
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

		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.beginRendering(rendering_info, FunctionDispatcher::get());
	}

	auto endRendering(CommandListHandle p_command_list) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.endRendering(FunctionDispatcher::get());
	}

	auto pushData(CommandListHandle p_command_list, const void *p_data, uint64 p_size, uint32 p_offset) -> void
	{
		vk::PushDataInfoEXT push_data_info{};
		push_data_info.data.address = p_data;
		push_data_info.data.size    = p_size;
		push_data_info.offset       = p_offset;

		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.pushDataEXT(push_data_info, FunctionDispatcher::get());
	}

	auto bindShaders(CommandListHandle p_command_list, InitialiserList<const ShaderHandle> p_shaders) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		std::vector<vk::ShaderStageFlagBits> stages(p_shaders.size());
		std::vector<vk::ShaderEXT>           shaders(p_shaders.size());
		for (uint32 i{0u}; i < p_shaders.size(); ++i)
		{
			const Shader &shader{g_impl->shaders[p_shaders[i]]};

			stages[i]  = shader.stage;
			shaders[i] = shader.shader;
		}

		cmd.cmd.bindShadersEXT(stages, shaders, FunctionDispatcher::get());
	}

	auto bindResourceHeap(CommandListHandle p_command_list, ResourceDescriptorHeapHandle p_resource_heap) -> void
	{
		CommandList &           cmd{g_impl->commandLists[p_command_list]};
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};
		cmd.cmd.bindResourceHeapEXT(resource_heap.bindInfo, FunctionDispatcher::get());
	}

	auto bindSamplerHeap(CommandListHandle p_command_list, SamplerDescriptorHeapHandle p_sampler_heap) -> void
	{
		CommandList &          cmd{g_impl->commandLists[p_command_list]};
		SamplerDescriptorHeap &sampler_heap{g_impl->samplerHeaps[p_sampler_heap]};
		cmd.cmd.bindSamplerHeapEXT(sampler_heap.bindInfo, FunctionDispatcher::get());
	}

	auto bindIndexBuffer(CommandListHandle p_command_list, BufferHandle p_index_buffer) -> void
	{
		CommandList & cmd{g_impl->commandLists[p_command_list]};
		const Buffer *index_buffer{g_impl->buffers.tryGet(p_index_buffer)};

		cmd.cmd.setVertexInputEXT({}, {}, FunctionDispatcher::get());

		if (index_buffer)
			cmd.cmd.bindIndexBuffer(index_buffer->buffer, 0u, vk::IndexType::eUint32, FunctionDispatcher::get());
		// else
		// cmd.cmd.bindIndexBuffer(nullptr, 0u, vk::IndexType::eUint32, FunctionDispatcher::get());
	}

	auto setPrimitiveTopology(CommandListHandle p_command_list, EPrimitiveTopology p_primitive_topology) -> void
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
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setPrimitiveTopologyEXT(primitive_topology, FunctionDispatcher::get());
	}

	auto setPrimitiveRestart(CommandListHandle p_command_list, bool p_enable, uint32 p_index) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setPrimitiveRestartEnableEXT(p_enable, FunctionDispatcher::get());
		if (p_enable)
			cmd.cmd.setPrimitiveRestartIndexEXT(p_index, FunctionDispatcher::get());
	}

	auto setViewport(CommandListHandle p_command_list, const tsm::Viewport &p_viewport) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setViewportWithCountEXT(vk::Viewport{
											p_viewport.offset.x,
											p_viewport.offset.y,
											p_viewport.size.x,
											p_viewport.size.y,
											p_viewport.depthBounds.x,
											p_viewport.depthBounds.y
										}, FunctionDispatcher::get());
	}

	auto setScissor(CommandListHandle p_command_list, const tsm::Rect &p_scissor_rect) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setScissorWithCountEXT(vk::Rect2D{{p_scissor_rect.offset.x, p_scissor_rect.offset.y}, {p_scissor_rect.size.x, p_scissor_rect.size.y}},
									   FunctionDispatcher::get());
	}

	auto setRasterizerDiscardEnable(CommandListHandle p_command_list, bool p_enable) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setRasterizerDiscardEnableEXT(p_enable, FunctionDispatcher::get());
	}

	auto setPolygonMode(CommandListHandle p_command_list, EPolygonMode p_polygon_mode) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

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

	auto setCullMode(CommandListHandle p_command_list, ECullMode p_cull_mode) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

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

	auto setFrontFace(CommandListHandle p_command_list, EFrontFace p_front_face) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		vk::FrontFace front_face{(p_front_face == EFrontFace::eCW) ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise};
		cmd.cmd.setFrontFaceEXT(front_face, FunctionDispatcher::get());
	}

	auto setDepthBias(CommandListHandle p_command_list, bool p_enable, float32 p_constant_factor, float32 p_clamp, float32 p_slope_factor) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		cmd.cmd.setDepthBiasEnableEXT(p_enable, FunctionDispatcher::get());
		if (p_enable)
			cmd.cmd.setDepthBias2EXT(vk::DepthBiasInfoEXT{p_constant_factor, p_clamp, p_slope_factor}, FunctionDispatcher::get());
	}

	auto setLineWidth(CommandListHandle p_command_list, float32 p_line_width) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setLineWidth(p_line_width, FunctionDispatcher::get());
	}

	auto setRasterizationSamples(CommandListHandle p_command_list, ESampleCount p_sample_count) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};

		vk::SampleCountFlagBits sample_count{getVulkanSampleCount(p_sample_count)};
		cmd.cmd.setRasterizationSamplesEXT(sample_count, FunctionDispatcher::get());

		cmd.cmd.setSampleMaskEXT(sample_count, 0xFFFFFFFF, FunctionDispatcher::get()); // I don't think I should expose this
		cmd.cmd.setAlphaToCoverageEnableEXT(false, FunctionDispatcher::get());         // I don't think I should expose this either
	}

	auto setDepthState(CommandListHandle p_command_list, bool p_test_enable, bool p_write_enable, bool p_clamp_enable, ECompareOp p_compare_op) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
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

	auto setStencilState(CommandListHandle p_command_list, bool p_test_enable) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.setStencilTestEnableEXT(p_test_enable, FunctionDispatcher::get());
	}

	auto draw(CommandListHandle p_command_list, uint32 p_vertex_count, uint32 p_instance_count, uint32 p_first_vertex, uint32 p_first_instance) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.draw(p_vertex_count, p_instance_count, p_first_vertex, p_first_instance, FunctionDispatcher::get());
	}

	auto drawIndexed(CommandListHandle p_command_list, uint32 p_index_count, uint32 p_instance_count, uint32 p_first_index, int32 p_vertex_offset,
					 uint32            p_first_instance) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		cmd.cmd.drawIndexed(p_index_count, p_instance_count, p_first_index, p_vertex_offset, p_first_instance, FunctionDispatcher::get());
	}

	auto drawIndirect(CommandListHandle p_command_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count, uint32 p_stride) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		Buffer &     buffer{g_impl->buffers[p_buffer]};
		cmd.cmd.drawIndirect(buffer.buffer, p_offset, p_draw_count, p_stride, FunctionDispatcher::get());
	}

	auto drawIndexedIndirect(CommandListHandle p_command_list, BufferHandle p_buffer, uint64 p_offset, uint32 p_draw_count, uint32 p_stride) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		Buffer &     buffer{g_impl->buffers[p_buffer]};
		cmd.cmd.drawIndexedIndirect(buffer.buffer, p_offset, p_draw_count, p_stride, FunctionDispatcher::get());
	}

	static auto getTextureBaseHeapSlot(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};
		return p_heap_slot + (resource_heap.imageSegmentOffset / resource_heap.imageDescriptorSize);
	}

	static auto getSegmentRelativeTextureHeapSlot(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};
		return p_heap_slot - (resource_heap.imageSegmentOffset / resource_heap.imageDescriptorSize);
	}

	auto createResourceDescriptorHeap(const ResourceDescriptorHeapDesc &p_desc) -> ResourceDescriptorHeapHandle
	{
		const auto &heap_props{g_impl->descriptorHeapProperties};

		ResourceDescriptorHeap resource_heap{};

		resource_heap.bufferSlotMutex = makeUnique<std::mutex>();
		resource_heap.imageSlotMutex  = makeUnique<std::mutex>();

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

		vmaCreateBuffer(g_impl->allocator, reinterpret_cast<VkBufferCreateInfo *>(&resource_heap_create_info), &heap_allocation_create_info,
						reinterpret_cast<VkBuffer *>(&resource_heap.heapBuffer), &resource_heap.heapAllocation, nullptr);

		vmaMapMemory(g_impl->allocator, resource_heap.heapAllocation, &resource_heap.mappedHeapMemory);

		vk::DeviceAddressRangeKHR device_address_range{};
		device_address_range.address               = g_impl->logicalDevice.getBufferAddress({resource_heap.heapBuffer});
		device_address_range.size                  = resource_heap_size;
		resource_heap.bindInfo.heapRange           = device_address_range;
		resource_heap.bindInfo.reservedRangeOffset = resource_heap_size - heap_props.minResourceHeapReservedRange;
		resource_heap.bindInfo.reservedRangeSize   = heap_props.minResourceHeapReservedRange;

		return g_impl->resourceHeaps.emplace(std::move(resource_heap));
	}

	auto createSamplerDescriptorHeap(const SamplerDescriptorHeapDesc &p_desc) -> SamplerDescriptorHeapHandle
	{
		const auto &heap_props{g_impl->descriptorHeapProperties};

		SamplerDescriptorHeap sampler_heap{};

		sampler_heap.samplerSlotMutex = makeUnique<std::mutex>();

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

		vmaCreateBuffer(g_impl->allocator, reinterpret_cast<VkBufferCreateInfo *>(&sampler_heap_create_info), &heap_allocation_create_info,
						reinterpret_cast<VkBuffer *>(&sampler_heap.heapBuffer), &sampler_heap.heapAllocation, nullptr);

		vmaMapMemory(g_impl->allocator, sampler_heap.heapAllocation, &sampler_heap.mappedHeapMemory);

		vk::DeviceAddressRangeKHR device_address_range{};
		device_address_range.address              = g_impl->logicalDevice.getBufferAddress({sampler_heap.heapBuffer});
		device_address_range.size                 = sampler_heap_size;
		sampler_heap.bindInfo.heapRange           = device_address_range;
		sampler_heap.bindInfo.reservedRangeOffset = sampler_heap_size - heap_props.minSamplerHeapReservedRange;
		sampler_heap.bindInfo.reservedRangeSize   = heap_props.minSamplerHeapReservedRange;

		return g_impl->samplerHeaps.emplace(std::move(sampler_heap));
	}

	auto destroyResourceDescriptorHeap(ResourceDescriptorHeapHandle p_resource_heap) -> void
	{
		g_impl->resourceHeaps.destroy(p_resource_heap);
	}

	auto destroySamplerDescriptorHeap(SamplerDescriptorHeapHandle p_sampler_heap) -> void
	{
		g_impl->samplerHeaps.destroy(p_sampler_heap);
	}

	auto allocBufferHeapSlot(ResourceDescriptorHeapHandle p_resource_heap) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};

		uint32 slot;
		{
			std::scoped_lock<std::mutex> lock{*resource_heap.bufferSlotMutex};
			slot = resource_heap.bufferSlotAllocator.allocSlot();
		}

		return slot;
	}

	auto allocTextureHeapSlot(ResourceDescriptorHeapHandle p_resource_heap) -> uint32
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};

		uint32 slot;
		{
			std::scoped_lock<std::mutex> lock{*resource_heap.imageSlotMutex};
			slot = resource_heap.imageSlotAllocator.allocSlot();
		}

		return getTextureBaseHeapSlot(p_resource_heap, slot);
	}

	auto allocSamplerHeapSlot(SamplerDescriptorHeapHandle p_sampler_heap) -> uint32
	{
		SamplerDescriptorHeap &sampler_heap{g_impl->samplerHeaps[p_sampler_heap]};

		uint32 slot;
		{
			std::scoped_lock<std::mutex> lock{*sampler_heap.samplerSlotMutex};
			slot = sampler_heap.samplerSlotAllocator.allocSlot();
		}

		return slot;
	}

	auto freeBufferHeapSlot(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};

		{
			std::scoped_lock<std::mutex> lock{*resource_heap.bufferSlotMutex};
			resource_heap.bufferSlotAllocator.freeSlot(p_heap_slot);
		}
	}

	auto freeTextureHeapSlot(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot) -> void
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};

		uint32 real_heap_slot{getSegmentRelativeTextureHeapSlot(p_resource_heap, p_heap_slot)};

		{
			std::scoped_lock<std::mutex> lock{*resource_heap.imageSlotMutex};
			resource_heap.imageSlotAllocator.freeSlot(real_heap_slot);
		}
	}

	auto freeSamplerHeapSlot(SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot) -> void
	{
		SamplerDescriptorHeap &sampler_heap{g_impl->samplerHeaps[p_sampler_heap]};

		{
			std::scoped_lock<std::mutex> lock{*sampler_heap.samplerSlotMutex};
			sampler_heap.samplerSlotAllocator.freeSlot(p_heap_slot);
		}
	}

	auto writeBufferDescriptor(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, BufferHandle p_buffer) -> void
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};
		Buffer &                buffer{g_impl->buffers[p_buffer]};

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

		g_impl->logicalDevice.writeResourceDescriptorsEXT(resource_info, host_range, FunctionDispatcher::get());
	}

	auto writeTextureDescriptor(ResourceDescriptorHeapHandle p_resource_heap, uint32 p_heap_slot, TextureHandle p_texture, bool p_storage, uint32 p_mip) -> void
	{
		ResourceDescriptorHeap &resource_heap{g_impl->resourceHeaps[p_resource_heap]};
		Texture &               texture{g_impl->textures[p_texture]};

		vk::ImageViewType image_view_type{vk::ImageViewType::e2D};

		switch (texture.desc.type)
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

		vk::Format image_format{getVulkanFormat(texture.desc.format)};

		vk::ImageViewCreateInfo image_view_create_info{};
		image_view_create_info.image      = texture.image;
		image_view_create_info.viewType   = image_view_type;
		image_view_create_info.format     = image_format;
		image_view_create_info.components = vk::ComponentMapping{
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		};
		image_view_create_info.subresourceRange = vk::ImageSubresourceRange{
			getImageAspectMask(texture.desc.format),
			(p_mip == UINT32_MAX) ? 0u : p_mip,
			(p_mip == UINT32_MAX) ? texture.desc.mipCount : 1u,
			0u,
			texture.desc.layerCount
		};

		uint32 real_heap_slot{getSegmentRelativeTextureHeapSlot(p_resource_heap, p_heap_slot)};

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

		g_impl->logicalDevice.writeResourceDescriptorsEXT(resource_info, host_range, FunctionDispatcher::get());
	}

	auto writeSamplerDescriptor(SamplerDescriptorHeapHandle p_sampler_heap, uint32 p_heap_slot, SamplerHandle p_sampler) -> void
	{
		SamplerDescriptorHeap &sampler_heap{g_impl->samplerHeaps[p_sampler_heap]};
		Sampler &              sampler{g_impl->samplers[p_sampler]};

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

		sampler_create_info.mipLodBias              = 0.0f;
		sampler_create_info.anisotropyEnable        = true;
		sampler_create_info.maxAnisotropy           = g_impl->physicalDevice.getProperties2().properties.limits.maxSamplerAnisotropy;
		sampler_create_info.compareEnable           = false;
		sampler_create_info.compareOp               = vk::CompareOp::eNever;
		sampler_create_info.minLod                  = 0.0f;
		sampler_create_info.maxLod                  = vk::LodClampNone;
		sampler_create_info.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
		sampler_create_info.unnormalizedCoordinates = false;

		vk::HostAddressRangeEXT host_range{};
		host_range.address = reinterpret_cast<void *>(reinterpret_cast<uint64>(sampler_heap.mappedHeapMemory) + static_cast<uint64>(p_heap_slot) * sampler_heap.
													  samplerDescriptorSize);
		host_range.size = sampler_heap.samplerDescriptorSize;

		g_impl->logicalDevice.writeSamplerDescriptorsEXT(sampler_create_info, host_range, FunctionDispatcher::get());
	}

	auto createBuffer(const BufferDesc &p_desc) -> BufferHandle
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

		std::array<uint32, 2u> queue_family_indices{g_impl->queueFamilyIndices.graphics, g_impl->queueFamilyIndices.transfer};
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
		vmaCreateBuffer(g_impl->allocator, reinterpret_cast<const VkBufferCreateInfo *>(&buffer_create_info), &allocation_create_info,
						reinterpret_cast<VkBuffer *>(&buffer), &allocation, &allocation_info);

		DeviceAddress device_address{0u};
		if (buffer_usage_flags & vk::BufferUsageFlagBits::eShaderDeviceAddress)
			device_address = g_impl->logicalDevice.getBufferAddress({buffer});

		return g_impl->buffers.emplace(Buffer{buffer, allocation, p_desc.size, device_address, allocation_info.pMappedData, p_desc.usage});
	}

	auto destroyBuffer(BufferHandle p_buffer) -> void
	{
		g_impl->buffers.destroy(p_buffer);
	}

	auto writeBufferData(BufferHandle p_buffer, const void *p_data, uint64 p_size, uint64 p_offset) -> void
	{
		Buffer &buffer{g_impl->buffers[p_buffer]};
		TST_ASSERT_MSG(buffer.mapped != nullptr, "Buffer is not host visible");
		TST_ASSERT_MSG(p_offset + p_size <= buffer.size, "Buffer write exceeds allocation");
		std::memcpy(static_cast<uint8 *>(buffer.mapped) + p_offset, p_data, p_size);
	}

	auto getBufferMappedData(BufferHandle p_buffer) -> void *
	{
		Buffer &buffer{g_impl->buffers[p_buffer]};
		TST_ASSERT_MSG(buffer.mapped != nullptr, "Buffer is not host visible");
		return buffer.mapped;
	}

	auto getBufferAddress(BufferHandle p_buffer) -> DeviceAddress
	{
		Buffer &buffer{g_impl->buffers[p_buffer]};
		return buffer.address;
	}

	auto createSurface(void *p_hwnd) -> SurfaceHandle
	{
		if (!g_impl->usingSwapchain)
		{
			TST_PERMA_ASSERT_MSG(false, "I can only create surfaces if you are using the swapchain");
			return {};
		}

		HWND hwnd{static_cast<HWND>(p_hwnd)};

		vk::Win32SurfaceCreateInfoKHR surface_create_info{};
		surface_create_info.hwnd      = hwnd;
		surface_create_info.hinstance = GetModuleHandle(nullptr);
		vk::SurfaceKHR vulkan_surface{g_impl->vulkanInstance.createWin32SurfaceKHR(surface_create_info)};

		Surface surface{};
		surface.surface = vulkan_surface;
		surface.hwnd    = hwnd;

		return g_impl->surfaces.emplace(surface);
	}

	auto destroySurface(SurfaceHandle p_surface) -> void
	{
		g_impl->surfaces.destroy(p_surface);
	}

	auto createSemaphore(uint64 p_initial_value) -> SemaphoreHandle
	{
		vk::SemaphoreTypeCreateInfo timeline_semaphore_create_info{};
		timeline_semaphore_create_info.initialValue  = p_initial_value;
		timeline_semaphore_create_info.semaphoreType = vk::SemaphoreType::eTimeline;

		vk::SemaphoreCreateInfo create_info{};
		create_info.pNext = &timeline_semaphore_create_info;

		vk::Semaphore semaphore{g_impl->logicalDevice.createSemaphore(create_info)};

		return g_impl->semaphores.emplace(Semaphore{semaphore});
	}

	auto destroySemaphore(SemaphoreHandle p_semaphore) -> void
	{
		g_impl->semaphores.destroy(p_semaphore);
	}

	auto waitSemaphores(InitialiserList<const SemaphoreHandle> p_semaphores, InitialiserList<const uint64> p_values) -> void
	{
		TST_ASSERT(p_semaphores.size() == p_values.size());

		std::vector<vk::Semaphore> semaphores(p_semaphores.size());
		for (uint64 i{0u}; i < p_semaphores.size(); ++i)
			semaphores[i] = g_impl->semaphores[p_semaphores[i]].semaphore;

		vk::SemaphoreWaitInfo wait_info{};
		wait_info.setSemaphores(semaphores);
		wait_info.setValues(p_values);

		vk::Result res{g_impl->logicalDevice.waitSemaphores(wait_info, INFINITE)};
		if (res != vk::Result::eSuccess)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to wait on semaphores");
		}
	}

	auto getSemaphoreValue(SemaphoreHandle p_semaphore) -> uint64
	{
		uint64     value{0u};
		vk::Result res{g_impl->logicalDevice.getSemaphoreCounterValue(g_impl->semaphores[p_semaphore].semaphore, &value)};
		TST_ASSERT_MSG(res == vk::Result::eSuccess, "Failed to query timeline semaphore");
		return value;
	}

	auto createTexture(const TextureDesc &p_desc) -> TextureHandle
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

		std::array<uint32, 2u> queue_family_indices{g_impl->queueFamilyIndices.graphics, g_impl->queueFamilyIndices.transfer}; // Again, I am not doin allat
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

		vmaCreateImage(g_impl->allocator, reinterpret_cast<const VkImageCreateInfo *>(&image_create_info), &allocation_create_info, reinterpret_cast<VkImage *>(&image),
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

			image_view = g_impl->logicalDevice.createImageView(image_view_create_info);
		}

		TextureHandle texture_handle{g_impl->textures.emplace(Texture{image, image_view, allocation, p_desc, false})};

		std::scoped_lock<std::mutex> lock{g_impl->undefinedTexturesMutex};
		g_impl->undefinedTextures.insert(texture_handle);

		return texture_handle;
	}

	auto destroyTexture(TextureHandle p_texture) -> void
	{
		g_impl->textures.destroy(p_texture);
		std::scoped_lock<std::mutex> lock{g_impl->undefinedTexturesMutex};
		g_impl->undefinedTextures.erase(p_texture);
	}

	auto getTextureDesc(TextureHandle p_texture) -> const TextureDesc &
	{
		const Texture &texture{g_impl->textures[p_texture]};
		return texture.desc;
	}

	auto createSampler(const SamplerDesc &p_desc) -> SamplerHandle
	{
		return g_impl->samplers.emplace(Sampler{p_desc});
	}

	auto destroySampler(SamplerHandle p_sampler) -> void
	{
		g_impl->samplers.destroy(p_sampler);
	}

	auto createSwapchainObjects(Swapchain &p_swapchain, tsm::uint2 p_extent, vk::SwapchainKHR p_old_swapchain = nullptr) -> bool
	{
		vk::Extent2D swapchain_extent{};

		auto &     surface{g_impl->surfaces[p_swapchain.surface]};
		const auto surface_caps{g_impl->physicalDevice.getSurfaceCapabilitiesKHR(surface.surface)};

		const auto available_formats{g_impl->physicalDevice.getSurfaceFormatsKHR(surface.surface)};
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

		auto available_present_modes{g_impl->physicalDevice.getSurfacePresentModesKHR(surface.surface)};
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

		if (g_impl->logicalDevice.createSwapchainKHR(&swapchain_create_info, nullptr, &p_swapchain.swapchain) != vk::Result::eSuccess)
		{
			p_swapchain.swapchain = nullptr;
			return false;
		}

		p_swapchain.extent = {swapchain_extent.width, swapchain_extent.height};

		TextureDesc attachment_desc{};
		attachment_desc.type        = ETextureType::e2D;
		attachment_desc.extent      = {p_swapchain.extent, 1u};
		attachment_desc.mipCount    = 1u;
		attachment_desc.layerCount  = 1u;
		attachment_desc.sampleCount = ESampleCount::e1;
		attachment_desc.format      = getTstFormat(p_swapchain.surfaceFormat.format);
		attachment_desc.usage       = ETextureUsageFlagBits::eColourAttachment;

		auto images{g_impl->logicalDevice.getSwapchainImagesKHR(p_swapchain.swapchain)};

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

			vk::ImageView image_view{g_impl->logicalDevice.createImageView(image_view_create_info)};

			// I don't know why I am providing this, as there is no reason to do so. But I guess now you can query the swapchain's texture desc?...
			TextureDesc fake_desc{
				tsm::uint3{p_swapchain.extent, 1u},
				1u,
				1u,
				ETextureType::e2D,
				ESampleCount::e1,
				getTstFormat(p_swapchain.surfaceFormat.format),
				ETextureUsageFlagBits::eNone
			};
			p_swapchain.attachments[i] = g_impl->textures.emplace(Texture{images[i], image_view, nullptr, fake_desc, true});
		}

		p_swapchain.renderFinishedSemaphores.resize(images.size());
		for (auto &semaphore: p_swapchain.renderFinishedSemaphores)
			semaphore = g_impl->logicalDevice.createSemaphore({});

		return true;
	}

	auto destroySwapchainObjects(Swapchain &p_swapchain) -> void
	{
		for (auto &attachment: p_swapchain.attachments)
			g_impl->textures.destroy(attachment);
		p_swapchain.attachments.clear();

		for (auto &semaphore: p_swapchain.renderFinishedSemaphores)
			g_impl->logicalDevice.destroySemaphore(semaphore);
		p_swapchain.renderFinishedSemaphores.clear();
	}

	auto createSwapchain(SurfaceHandle p_surface, tsm::uint2 p_initial_extent) -> SwapchainHandle
	{
		Swapchain swapchain{};
		swapchain.surface = p_surface;
		swapchain.extent  = p_initial_extent;

		swapchain.imageAvailableSemaphores.resize(g_impl->maxConcurrentSwapchainWorkloads);
		for (auto &semaphore: swapchain.imageAvailableSemaphores)
			semaphore = g_impl->logicalDevice.createSemaphore({});

		createSwapchainObjects(swapchain, p_initial_extent);

		return g_impl->swapchains.emplace(std::move(swapchain));
	}

	auto resizeSwapchain(SwapchainHandle p_swapchain, tsm::uint2 p_extent) -> bool
	{
		g_impl->logicalDevice.waitIdle();

		Swapchain &swapchain{g_impl->swapchains[p_swapchain]};

		vk::SwapchainKHR old_swapchain{swapchain.swapchain};

		destroySwapchainObjects(swapchain);
		if (!createSwapchainObjects(swapchain, p_extent, old_swapchain)) // You can handle ts on your own
			return false;

		if (old_swapchain)
			g_impl->logicalDevice.destroySwapchainKHR(old_swapchain);

		return true;
	}

	auto destroySwapchain(SwapchainHandle p_swapchain) -> void
	{
		g_impl->swapchains.destroy(p_swapchain);
	}

	auto acquireNextImage(SwapchainHandle p_swapchain) -> TextureHandle
	{
		Swapchain &swapchain{g_impl->swapchains[p_swapchain]};
		if (!swapchain.swapchain)
			return nullptr;

		swapchain.acquisitonIndex = (swapchain.acquisitonIndex + 1u) % swapchain.imageAvailableSemaphores.size();

		auto [res, idx]{g_impl->logicalDevice.acquireNextImageKHR(swapchain.swapchain, INFINITE, swapchain.imageAvailableSemaphores[swapchain.acquisitonIndex], nullptr)};
		swapchain.imageIndex = idx;
		if (res != vk::Result::eSuccess)
			return nullptr;

		return swapchain.attachments[swapchain.imageIndex];
	}

	auto insertPreRenderSwapchainResourceBarrier(CommandListHandle p_command_list, TextureHandle p_attachment_texture) -> void
	{
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		TST_ASSERT(cmd.open);

		Texture &attachment{g_impl->textures[p_attachment_texture]};

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

	auto submitAndPresent(SwapchainHandle p_swapchain, CommandListHandle p_command_list, const SemaphoreSubmitInfo &p_signal_semaphore_info) -> bool
	{
		return submitAndPresent(p_swapchain, p_command_list, p_signal_semaphore_info, {});
	}

	auto submitAndPresent(SwapchainHandle                            p_swapchain, CommandListHandle p_command_list, const SemaphoreSubmitInfo &p_signal_semaphore_info,
						  InitialiserList<const SemaphoreSubmitInfo> p_wait_semaphore_infos) -> bool
	{
		Swapchain &  swapchain{g_impl->swapchains[p_swapchain]};
		CommandList &cmd{g_impl->commandLists[p_command_list]};
		TST_ASSERT(cmd.open);

		Texture &colour_attachment{g_impl->textures[swapchain.attachments[swapchain.imageIndex]]};

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

		closeCommandList(p_command_list);

		std::vector<vk::SemaphoreSubmitInfo> raw_waits;
		raw_waits.reserve(1u + p_wait_semaphore_infos.size());
		raw_waits.emplace_back(swapchain.imageAvailableSemaphores[swapchain.acquisitonIndex]);
		for (const SemaphoreSubmitInfo &wait: p_wait_semaphore_infos)
			raw_waits.emplace_back(g_impl->semaphores[wait.semaphore].semaphore, wait.value);

		submit(EQueueType::eGraphics, p_command_list, raw_waits, {
				   vk::SemaphoreSubmitInfo{swapchain.renderFinishedSemaphores[swapchain.imageIndex]},
				   vk::SemaphoreSubmitInfo{g_impl->semaphores[p_signal_semaphore_info.semaphore].semaphore, p_signal_semaphore_info.value}
			   });

		vk::PresentInfoKHR present_info{};
		present_info.setSwapchains(swapchain.swapchain);
		present_info.setImageIndices(swapchain.imageIndex);
		present_info.setWaitSemaphores(swapchain.renderFinishedSemaphores[swapchain.imageIndex]);

		// Avoids vulkan hpp exceptions
		const vk::Result res{
			FunctionDispatcher::get().vkQueuePresentKHR(g_impl->queues[static_cast<uint32>(EQueueType::eGraphics)],
														reinterpret_cast<const VkPresentInfoKHR *>(&present_info))
		};

		if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR)
			return false;
		else if (res != vk::Result::eSuccess)
			TST_PERMA_ASSERT_MSG(false, "Failed to present swapchain image!");

		return true;
	}

	auto createShader(const ShaderDesc &p_desc) -> ShaderHandle
	{
		vk::ShaderStageFlagBits shader_stage{static_cast<vk::ShaderStageFlagBits>(static_cast<uint32>(getVulkanShaderStages(p_desc.stage)))};
		vk::ShaderStageFlags    next_stage{getVulkanShaderStages(p_desc.nextStage)};

		vk::ShaderCreateInfoEXT shader_create_info{};
		shader_create_info.flags     = vk::ShaderCreateFlagBitsEXT::eDescriptorHeap;
		shader_create_info.stage     = shader_stage;
		shader_create_info.nextStage = next_stage;
		shader_create_info.codeType  = vk::ShaderCodeTypeEXT::eSpirv;
		shader_create_info.codeSize  = p_desc.codeSizeWords * sizeof(uint32);
		shader_create_info.pCode     = p_desc.code;
		shader_create_info.pName     = p_desc.entryPointName;

		auto [res, shader]{g_impl->logicalDevice.createShaderEXT(shader_create_info, nullptr, FunctionDispatcher::get())};
		if (res != vk::Result::eSuccess)
		{
			TST_PERMA_ASSERT_MSG(false, "Failed to create shader!");
		}

		return g_impl->shaders.emplace(Shader{shader, shader_stage, next_stage});
	}

	auto destroyShader(ShaderHandle p_shader) -> void
	{
		g_impl->shaders.destroy(p_shader);
	}
}
