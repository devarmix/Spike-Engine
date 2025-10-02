#pragma once

#include <Backends/Vulkan/VulkanCommon.h>

namespace Spike {

	struct VulkanRHITexture {

		VkImage Image = nullptr;
		VmaAllocation Allocation = nullptr;
	};

	struct VulkanRHITextureView {
		VkImageView View = nullptr;
	};

	struct VulkanRHISampler {
		VkSampler Sampler = nullptr;
	};

	struct VulkanRHIBuffer {

		VkBuffer Buffer = nullptr;
		VmaAllocation Allocation = nullptr;
		VmaAllocationInfo AllocationInfo;
	};

	struct VulkanRHICommandBuffer {

		VkCommandBuffer Cmd = nullptr;
		VkCommandPool Pool = nullptr;
	};

	struct VulkanRHIShader {

		VkPipeline Pipeline = nullptr;
		VkPipelineLayout PipelineLayout = nullptr;
	};

	struct VulkanRHIBindingSetLayout {
		VkDescriptorSetLayout Layout = nullptr;
	};

	struct VulkanRHIBindingSet {

		VkDescriptorSet Set = nullptr;
		VkDescriptorPool AllocatedPool = nullptr;
	};
}