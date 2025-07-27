#pragma once

#ifdef TST_PLATFORM_VULKAN
#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif

#include "platform/Windows/WindowsWindow.hpp"


namespace tst {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	uint32_t presentFamily;
	bool graphicsFamilyHasValue = false;
	bool presentFamilyHasValue = false;
	[[nodiscard]] bool isComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class Device {

public:


	explicit Device(GLFWwindow *window);
	~Device();

	// Not copyable or movable
	Device(const Device &other) = delete;
	Device(Device &&other) = delete;
	Device &operator=(const Device &other) = delete;
	Device &operator=(Device &&other) = delete;

	[[nodiscard]] VkCommandPool getCommandPool() const { return m_commandPool; }
	[[nodiscard]] VkDevice getDevice() const { return m_device; }
	[[nodiscard]] VkSurfaceKHR getSurface() const { return m_surface; }
	[[nodiscard]] VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
	[[nodiscard]] VkQueue getPresentQueue() const { return m_presentQueue; }

	SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_physicalDevice); }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); }
	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// Buffer Helper Functions
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
		VkDeviceMemory &bufferMemory);

	VkCommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

	void createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image,
		VkDeviceMemory &imageMemory);

	VkPhysicalDeviceProperties properties;

private:

	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	// helper functions

	bool isDeviceSuitable(VkPhysicalDevice device);
	std::vector<const char *> getRequiredExtensions();
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	void hasGlfwRequiredInstanceExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	GLFWwindow *m_window;
	VkCommandPool m_commandPool;

	VkDevice m_device;
	VkSurfaceKHR m_surface;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	};
}

#endif