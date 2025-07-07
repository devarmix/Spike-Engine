#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>

namespace Spike {

	struct DescriptorLayoutBuilder {

		std::vector<VkDescriptorSetLayoutBinding> Bindings;

		void AddBinding(uint32_t binding, VkDescriptorType type);
		void Clear();
		VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderFlags, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
	};

	struct DescriptorAllocator {

		struct PoolSizeRatio {

			VkDescriptorType Type;
			uint32_t Ratio;
		};

		VkDescriptorPool Pool;

		void InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
		void ClearDescriptors(VkDevice device);
		void DestroyPool(VkDevice device);

		VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
	};

	struct DescriptorWriter {

		std::deque<VkDescriptorImageInfo> ImageInfos;
		std::deque<VkDescriptorBufferInfo> BufferInfos;
		std::vector<VkWriteDescriptorSet> Writes;

		void WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
		void WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

		void Clear();
		void UpdateSet(VkDevice device, VkDescriptorSet set);
	};
}
