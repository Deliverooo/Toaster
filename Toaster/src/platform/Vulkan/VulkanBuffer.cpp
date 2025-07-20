#include "tstpch.h"
//#include "VulkanBuffer.hpp"
//
//
//namespace tst
//{
//
//	VulkanBuffer::VulkanBuffer(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags,
//		VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment) : m_instanceCount(instanceCount), m_instanceSize(instanceSize),
//		m_usageFlags(usageFlags), m_memoryPropertyFlags(memoryPropertyFlags) {
//
//		m_alignmentSize = (minOffsetAlignment > 0) ? (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1) : instanceSize;
//		m_bufferSize = m_alignmentSize * instanceCount;
//		/*device.createBuffer(m_bufferSize, usageFlags, memoryPropertyFlags, m_buffer, m_memory);*/
//	}
//
//}