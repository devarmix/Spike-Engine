#pragma once

#include <Platforms/Vulkan/VulkanBuffer.h>

#include <Platforms/Vulkan/VulkanRenderResource.h>
#include <Platforms/Vulkan/VulkanShader.h>
#include <Engine/Renderer/Material.h>

namespace Spike {

	struct VulkanMaterialManager {

		VkDescriptorPool DataPool;
		VkDescriptorSet DataSet;
		VkDescriptorSetLayout DataSetLayout;

		VulkanBuffer DataBuffer;

		static constexpr uint32_t IndexInvalid = 10000000;

		std::deque<uint32_t> FreeTextureIndicies;
		std::deque<uint32_t> FreeBufferIndicies;

		uint32_t NextTextureIndex;
		uint32_t NextBufferIndex;

		uint32_t GetFreeTextureIndex();
		uint32_t GetFreeBufferIndex();

		void ReleaseTextureIndex(uint32_t index);
		void ReleaseBufferIndex(uint32_t index);

		void WriteTexture(VulkanDevice* device, uint32_t index, VulkanTextureNativeData* texData, VkSampler sampler);
		void WriteBuffer(VulkanDevice* device, uint32_t index);

		void Init(VulkanDevice* device, uint32_t maxMatSets);
		void Cleanup(VulkanDevice* device);

		void BuildMaterialPipeline(VkPipeline* outPipeline, VkPipelineLayout* outLayout, VulkanDevice* device, VkDescriptorSetLayout frameSetLayout, VulkanShader shader, EMaterialSurfaceType surfaceType);

		struct alignas(16) MaterialPushConstants {

			uint32_t SceneDataOffset;
			float pad0[3];
		};

		struct alignas(16) MaterialDataConstants {

			glm::vec4 ColorData[16];
			float ScalarData[48];

			uint32_t TexIndex[16];
		};
	};
}
