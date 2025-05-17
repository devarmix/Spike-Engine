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

		VulkanCubeTexture(const VulkanCubeTextureData& data) : m_Data(data) {}
		virtual ~VulkanCubeTexture() override { Destroy(); ASSET_CORE_DESTROY(); }

		void Destroy();

		virtual const void* GetData() const override { return (void*)&m_Data; }
		const VulkanCubeTextureData* GetRawData() const { return &m_Data; }

		virtual const Vector3 GetSize() const override { return Vector3((float)m_Data.Extent.width, (float)m_Data.Extent.height, (float)m_Data.Extent.depth); }

		static Ref<VulkanCubeTexture> Create(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		static Ref<VulkanCubeTexture> Create(const std::array<void*, 6>& data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		static Ref<VulkanCubeTexture> Create(const std::array<const char*, 6>& filePath);

		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		static void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

	private:

		VulkanCubeTextureData m_Data;
	};
}
