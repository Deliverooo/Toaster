#pragma once
#include <vulkan/vulkan_core.h>

#include "VulkanDevice.hpp"
#include "platform/Windows/WindowsWindow.hpp"
#include "Toaster/Core/Window.hpp"
#include "Toaster/Renderer/RenderingContext.hpp"

namespace tst
{

	class TST_API VulkanRenderingContext : public RenderingContext
	{
	public:

		VulkanRenderingContext(GLFWwindow* window);
		virtual ~VulkanRenderingContext() override;


		virtual void init() override;
		virtual void swapBuffers() override;

		static VkInstance GetInstance();
		static VkPhysicalDevice GetPhysicalDevice();
		static VkDevice GetDevice();

		static VkCommandBuffer GetCommandBuffer(bool begin);
		static void FlushCommandBuffer(VkCommandBuffer commandBuffer);

	private:
		GLFWwindow* m_window;
		
		Device m_device{ m_window };
	};
}
