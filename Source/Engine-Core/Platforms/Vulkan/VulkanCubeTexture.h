#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Engine/Renderer/CubeTexture.h>
using namespace SpikeEngine;

namespace Spike {

	struct VulkanCubeTextureMipmaper {

		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;

		VkDescriptorSet Set;
		VkDescriptorSetLayout SetLayout;

		struct PushConstants {

			float Roughness;
			float extra[3];
		};

		void Init();
		void Cleanup();

		void GenerateMipmap(VkCommandBuffer cmd, VkImage image, VkExtent2D size, uint32_t currMip, uint32_t numMips, uint32_t currSide);
	};

	struct VulkanCubeTextureData {

		VkImage Image;
		VkImageView View;
		VmaAllocation Allocation;
		VkExtent3D Extent;
		VkFormat Format;
	};

	class VulkanCubeTexture : public CubeTexture {
	public:

		VulkanCubeTexture() = default;
		virtual ~VulkanCubeTexture() override { Destroy(); ASSET_CORE_DESTROY(); }

		void Destroy();

		virtual void* GetData() override { return &Data; }
		virtual Vector3 GetSize() const override { return Vector3((float)Data.Extent.width, (float)Data.Extent.height, (float)Data.Extent.depth); }

		static Ref<VulkanCubeTexture> Create(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		static Ref<VulkanCubeTexture> Create(void** data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		static Ref<VulkanCubeTexture> Create(const char** filePath);

		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		static void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

	public:

		VulkanCubeTextureData Data;
	};
}
