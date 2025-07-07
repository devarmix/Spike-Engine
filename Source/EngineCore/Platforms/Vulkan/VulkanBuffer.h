#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Platforms/Vulkan/VulkanDevice.h>
#include <Engine/Core/Core.h>

namespace Spike {

	class VulkanBuffer {
	public:
		VulkanBuffer();
		VulkanBuffer(VulkanDevice* device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);

		~VulkanBuffer() {}

		void Destroy(VulkanDevice* device);

	public:

		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
	};
}
