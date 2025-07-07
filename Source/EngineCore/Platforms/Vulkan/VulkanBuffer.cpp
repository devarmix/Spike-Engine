#include <Platforms/Vulkan/VulkanBuffer.h>

namespace Spike {

	VulkanBuffer::VulkanBuffer() :
		Buffer(nullptr),
		Allocation(nullptr),
		AllocationInfo{} {}

	VulkanBuffer::VulkanBuffer(VulkanDevice* device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {

		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = memUsage;
		vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_CHECK(vmaCreateBuffer(device->Allocator, &bufferInfo, &vmaAllocInfo, &Buffer,
			&Allocation, &AllocationInfo));
	}

	void VulkanBuffer::Destroy(VulkanDevice* device) {

		if (Buffer != VK_NULL_HANDLE) {

			vmaDestroyBuffer(device->Allocator, Buffer, Allocation);
			Buffer = VK_NULL_HANDLE;
		}
	}
}