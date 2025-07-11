#include <Platforms/Vulkan/VulkanMaterial.h>
#include <Platforms/Vulkan/VulkanDescriptors.h>

#include <Platforms/Vulkan/VulkanPipeline.h>
#include <Platforms/Vulkan/VulkanTools.h>

namespace Spike {

	void VulkanMaterialManager::Init(VulkanDevice* device, uint32_t maxMatSets) {

		// Create Descriptor Set Layout
		{
			std::array<VkDescriptorSetLayoutBinding, 2> LayoutBindings{

				VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = maxMatSets * 16, .stageFlags = VK_SHADER_STAGE_ALL, .pImmutableSamplers = nullptr},

				VkDescriptorSetLayoutBinding{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = maxMatSets, .stageFlags = VK_SHADER_STAGE_ALL, .pImmutableSamplers = nullptr}
			};

			std::array<VkDescriptorBindingFlags, 2> LayoutBindingFlags{

				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
			};

			VkDescriptorSetLayoutBindingFlagsCreateInfo LayoutBindingFlagsInfo{};
			LayoutBindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			LayoutBindingFlagsInfo.pNext = nullptr;
			LayoutBindingFlagsInfo.pBindingFlags = LayoutBindingFlags.data();
			LayoutBindingFlagsInfo.bindingCount = 2;

			VkDescriptorSetLayoutCreateInfo LayoutInfo{};
			LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutInfo.bindingCount = 2;
			LayoutInfo.pBindings = LayoutBindings.data();

			LayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			LayoutInfo.pNext = &LayoutBindingFlagsInfo;

			VK_CHECK(vkCreateDescriptorSetLayout(device->Device, &LayoutInfo, nullptr, &DataSetLayout));
		}

		// Create Descriptor Pool
		{
			std::array<VkDescriptorPoolSize, 2> PoolSizes {

				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = maxMatSets * 16},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = maxMatSets}
			};

			VkDescriptorPoolCreateInfo PoolInfo{};
			PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			PoolInfo.poolSizeCount = 2;
			PoolInfo.pPoolSizes = PoolSizes.data();
			PoolInfo.maxSets = 1;
			PoolInfo.pNext = nullptr;

			VK_CHECK(vkCreateDescriptorPool(device->Device, &PoolInfo, nullptr, &DataPool));
		}

		// Create Descriptor Set
		{
			VkDescriptorSetAllocateInfo SetInfo{};
			SetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			SetInfo.pNext = nullptr;
			SetInfo.descriptorPool = DataPool;
			SetInfo.pSetLayouts = &DataSetLayout;
			SetInfo.descriptorSetCount = 1;

			VK_CHECK(vkAllocateDescriptorSets(device->Device, &SetInfo, &DataSet));
		}

		// Allocate Data Buffer
		DataBuffer = VulkanBuffer(device, sizeof(MaterialDataConstants) * maxMatSets, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Initialize Other Parameters
		NextBufferIndex = 0;
		NextTextureIndex = 0;

		FreeBufferIndicies.clear();
		FreeTextureIndicies.clear();
	}

	void VulkanMaterialManager::Cleanup(VulkanDevice* device) {

		vkDestroyDescriptorPool(device->Device, DataPool, nullptr);

		vkDestroyDescriptorSetLayout(device->Device, DataSetLayout, nullptr);

		DataBuffer.Destroy(device);
	}

	void VulkanMaterialManager::BuildMaterialPipeline(VkPipeline* outPipeline, VkPipelineLayout* outLayout, VulkanDevice* device, VkDescriptorSetLayout frameSetLayout, VulkanShader shader, EMaterialSurfaceType surfaceType) {

		VulkanTools::VulkanGraphicsPipelineInfo info{};
		info.VertexModule = shader.VertexModule;
		info.FragmentModule = shader.FragmentModule;

		VkDescriptorSetLayout layouts[2] = { frameSetLayout, DataSetLayout };
		info.SetLayouts = layouts;
		info.SetLayoutsCount = 2;
		info.PushConstantSize = sizeof(MaterialPushConstants);

		VkFormat colorAttachmentFormats[3] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM };
		info.ColorAttachmentFormats = colorAttachmentFormats;
		info.ColorAttachmentCount = 3;
		info.EnableDepthTest = true;
		info.EnableDepthWrite = true;
		info.DepthCompare = VK_COMPARE_OP_GREATER_OR_EQUAL;
		info.CullMode = VK_CULL_MODE_BACK_BIT;
		info.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VulkanTools::CreateVulkanGraphicsPipeline(device->Device, info, outPipeline, outLayout);
	}

	uint32_t VulkanMaterialManager::GetFreeTextureIndex() {

		if (FreeTextureIndicies.size() > 0) {

			uint32_t index = FreeTextureIndicies.front();
			FreeTextureIndicies.pop_front();

			return index;
		}

		uint32_t index = NextTextureIndex;
		NextTextureIndex++;

		return index;
	}

	uint32_t VulkanMaterialManager::GetFreeBufferIndex() {

		if (FreeBufferIndicies.size() > 0) {

			uint32_t index = FreeBufferIndicies.front();
			FreeBufferIndicies.pop_front();

			return index;
		}

		uint32_t index = NextBufferIndex;
		NextBufferIndex++;

		return index;
	}

	void VulkanMaterialManager::ReleaseTextureIndex(uint32_t index) {

		// TODO: Check. if the index already is in deque, to prevent data overwriting

		FreeTextureIndicies.push_back(index);
	}

	void VulkanMaterialManager::ReleaseBufferIndex(uint32_t index) {

		// TODO: Check, if the index already is in deque, to prevent data overwriting

		FreeBufferIndicies.push_back(index);
	}

	void VulkanMaterialManager::WriteBuffer(VulkanDevice* device, uint32_t index) {

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = DataBuffer.Buffer;
		bufferInfo.offset = index * sizeof(MaterialDataConstants);
		bufferInfo.range = sizeof(MaterialDataConstants);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = DataSet;
		write.dstBinding = 1;
		write.dstArrayElement = index;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device->Device, 1, &write, 0, nullptr);
	}

	void VulkanMaterialManager::WriteTexture(VulkanDevice* device, uint32_t index, VulkanTextureNativeData* texData, VkSampler sampler) {

		assert(index != IndexInvalid);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texData->View;
		imageInfo.sampler = sampler;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = DataSet;
		write.dstBinding = 0;
		write.dstArrayElement = index;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device->Device, 1, &write, 0, nullptr);
	}
}