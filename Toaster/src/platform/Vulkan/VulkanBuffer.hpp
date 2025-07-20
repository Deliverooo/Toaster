//#pragma once
//#include "Toaster/Renderer/Buffer.hpp"
//#include <vulkan/vulkan.h>
//
//namespace tst
//{
//	class TST_API VulkanBuffer
//	{
//	public:
//
//		virtual ~VulkanBuffer() {}
//
//		VulkanBuffer(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags,
//			VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment = 1);
//
//		VkBuffer getBuffer() const { return m_buffer; }
//		void* getMappedMemory() const { return m_mappedMemory; }
//		uint32_t getInstanceCount() const { return m_instanceCount; }
//		VkDeviceSize getInstanceSize() const { return m_instanceSize; }
//		VkDeviceSize getAlignmentSize() const { return m_alignmentSize; }
//		VkBufferUsageFlags getUsageFlags() const { return m_usageFlags; }
//		VkMemoryPropertyFlags getMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
//		VkDeviceSize getBufferSize() const { return m_bufferSize; }
//
//	protected:
//		void* m_mappedMemory = nullptr;
//		VkBuffer m_buffer = VK_NULL_HANDLE;
//		VkDeviceMemory m_memory = VK_NULL_HANDLE;
//
//		VkDeviceSize m_bufferSize;
//		uint32_t m_instanceCount;
//		VkDeviceSize m_instanceSize;
//		VkDeviceSize m_alignmentSize;
//		VkBufferUsageFlags m_usageFlags;
//		VkMemoryPropertyFlags m_memoryPropertyFlags;
//
//	};
//	class TST_API VulkanVertexBuffer : public VertexBuffer, public VulkanBuffer
//	{
//	public:
//		VulkanVertexBuffer(float* vertices, uint32_t count);
//		virtual ~VulkanVertexBuffer() override;
//
//		virtual void bind() const override;
//		virtual void unbind() const override;
//
//		virtual void setLayout(const BufferLayout& layout) override;
//		virtual const BufferLayout &getLayout() const override;
//
//	private:
//		unsigned int m_Vbo;
//		BufferLayout m_bufferLayout;
//		uint32_t m_count;
//	};
//
//	class TST_API VulkanIndexBuffer : public VulkanBuffer, public IndexBuffer
//	{
//	public:
//		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
//		virtual ~VulkanIndexBuffer() override;
//
//		virtual void bind() const override;
//		virtual void unbind() const override;
//
//		virtual uint32_t count() const override;
//
//	private:
//		unsigned int m_Ebo;
//		uint32_t m_count;
//	};
//
//}
//
