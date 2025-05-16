#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Engine/Core/Core.h>

namespace Spike {

	struct VulkanBuffer {

		VulkanBuffer() = default;

		void Destroy();

		static VulkanBuffer Create(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);

		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
	};
}
