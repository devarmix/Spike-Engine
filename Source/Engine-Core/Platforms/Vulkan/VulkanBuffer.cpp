#include <Platforms/Vulkan/VulkanBuffer.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

namespace Spike {

	VulkanBuffer VulkanBuffer::Create(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {

		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = memUsage;
		vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VulkanBuffer newBuffer;

		VK_CHECK(vmaCreateBuffer(VulkanRenderer::Device.Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer,
			&newBuffer.Allocation, &newBuffer.AllocationInfo));

		return newBuffer;
	}

	void VulkanBuffer::Destroy() {

		if (Buffer != VK_NULL_HANDLE) {

			vmaDestroyBuffer(VulkanRenderer::Device.Allocator, Buffer, Allocation);
			Buffer = VK_NULL_HANDLE;
		}
	}
}