#include <Platforms/Vulkan/VulkanDescriptors.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

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


	VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice device) {

		VkDescriptorPool newPool;
		if (ReadyPools.size() != 0) {

			newPool = ReadyPools.back();
			ReadyPools.pop_back();
		}
		else {
			// need to create a new pool
			newPool = CreatePool(device, SetsPerPool, Ratios);

			SetsPerPool = uint32_t(SetsPerPool * 1.5);
			if (SetsPerPool > 4092) {
				SetsPerPool = 4092;
			}
		}

		return newPool;
	}

	VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {

		std::vector<VkDescriptorPoolSize> poolSizes;
		for (PoolSizeRatio ratio : poolRatios) {

			poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.Type,
				.descriptorCount = uint32_t(ratio.Ratio * setCount)
				});
		}

		VkDescriptorPoolCreateInfo pool_Info = {};
		pool_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_Info.flags = 0;
		pool_Info.maxSets = setCount;
		pool_Info.poolSizeCount = (uint32_t)poolSizes.size();
		pool_Info.pPoolSizes = poolSizes.data();

		VkDescriptorPool newPool;
		vkCreateDescriptorPool(device, &pool_Info, nullptr, &newPool);

		return newPool;
	}

	void DescriptorAllocatorGrowable::Init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {

		Ratios.clear();

		for (auto r : poolRatios) {
			Ratios.push_back(r);
		}

		VkDescriptorPool newPool = CreatePool(device, maxSets, poolRatios);

		SetsPerPool = uint32_t(maxSets * 1.5);

		ReadyPools.push_back(newPool);
	}

	void DescriptorAllocatorGrowable::ClearPools(VkDevice device) {

		for (auto p : ReadyPools) {
			vkResetDescriptorPool(device, p, 0);
		}

		for (auto p : FullPools) {

			vkResetDescriptorPool(device, p, 0);
			ReadyPools.push_back(p);
		}

		FullPools.clear();
	}

	void DescriptorAllocatorGrowable::DestroyPools(VkDevice device) {

		for (auto p : ReadyPools) {
			vkDestroyDescriptorPool(device, p, nullptr);
		}
		ReadyPools.clear();

		for (auto p : FullPools) {
			vkDestroyDescriptorPool(device, p, nullptr);
		}
		FullPools.clear();
	}

	VkDescriptorSet DescriptorAllocatorGrowable::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext) {

		// get or create a pool to allocate from
		VkDescriptorPool poolToUse = GetPool(device);

		VkDescriptorSetAllocateInfo alloc_Info = {};
		alloc_Info.pNext = pNext;
		alloc_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_Info.descriptorPool = poolToUse;
		alloc_Info.descriptorSetCount = 1;
		alloc_Info.pSetLayouts = &layout;

		VkDescriptorSet ds;
		VkResult result = vkAllocateDescriptorSets(device, &alloc_Info, &ds);

		// allocation failed. Try again
		if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

			FullPools.push_back(poolToUse);

			poolToUse = GetPool(device);
			alloc_Info.descriptorPool = poolToUse;

			VK_CHECK(vkAllocateDescriptorSets(device, &alloc_Info, &ds));
		}

		ReadyPools.push_back(poolToUse);
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