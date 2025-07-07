#include <Platforms/Vulkan/VulkanDescriptors.h>

namespace Spike {

	void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type) {

		VkDescriptorSetLayoutBinding newbind{};
		newbind.binding = binding;
		newbind.descriptorCount = 1;
		newbind.descriptorType = type;

		Bindings.push_back(newbind);
	}

	void DescriptorLayoutBuilder::Clear() {

		Bindings.clear();
	}

	VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderFlags, void* pNext, VkDescriptorSetLayoutCreateFlags flags) {

		for (auto& b : Bindings) {
			b.stageFlags |= shaderFlags;
		}

		VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.pNext = pNext;

		info.pBindings = Bindings.data();
		info.bindingCount = (uint32_t)Bindings.size();
		info.flags = flags;

		VkDescriptorSetLayout set;
		VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

		return set;
	}

	void DescriptorAllocator::InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {

		std::vector<VkDescriptorPoolSize> poolSizes;
		for (PoolSizeRatio ratio : poolRatios) {
			poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.Type,
				.descriptorCount = ratio.Ratio
				});
		}

		VkDescriptorPoolCreateInfo pool_Info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_Info.flags = 0;
		pool_Info.maxSets = maxSets;
		pool_Info.poolSizeCount = (uint32_t)poolSizes.size();
		pool_Info.pPoolSizes = poolSizes.data();

		vkCreateDescriptorPool(device, &pool_Info, nullptr, &Pool);
	}

	void DescriptorAllocator::ClearDescriptors(VkDevice device) {

		vkResetDescriptorPool(device, Pool, 0);
	}

	void DescriptorAllocator::DestroyPool(VkDevice device) {

		vkDestroyDescriptorPool(device, Pool, nullptr);
	}

	VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout) {

		VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = Pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet ds;
		VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

		return ds;
	}


	void DescriptorWriter::WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {

		VkDescriptorImageInfo& info = ImageInfos.emplace_back(VkDescriptorImageInfo{
			.sampler = sampler,
			.imageView = image,
			.imageLayout = layout
			});

		VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

		write.dstBinding = binding;
		write.dstSet = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = &info;

		Writes.push_back(write);
	}

	void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {

		VkDescriptorBufferInfo& info = BufferInfos.emplace_back(VkDescriptorBufferInfo{
			.buffer = buffer,
			.offset = offset,
			.range = size
			});

		VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

		write.dstBinding = binding;
		write.dstSet = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &info;

		Writes.push_back(write);
	}

	void DescriptorWriter::Clear() {

		ImageInfos.clear();
		Writes.clear();
		BufferInfos.clear();
	}

	void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set) {

		for (VkWriteDescriptorSet& write : Writes) {
			write.dstSet = set;
		}

		vkUpdateDescriptorSets(device, (uint32_t)Writes.size(), Writes.data(), 0, nullptr);
	}
}