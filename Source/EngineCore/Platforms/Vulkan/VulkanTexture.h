#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Engine/Renderer/Texture.h>
using namespace SpikeEngine;

namespace Spike {

	struct VulkanTextureData {

		VkImage Image;
		VkImageView View;
		VmaAllocation Allocation;
		VkExtent3D Extent;
		VkFormat Format;
	};

	class VulkanTexture : public Texture {
	public:

		VulkanTexture(const VulkanTextureData& data) : m_Data(data) {}
		virtual ~VulkanTexture() override { Destroy(); ASSET_CORE_DESTROY(); }

		void Destroy();

		virtual const void* GetData() const override { return (void*)&m_Data; }
		const VulkanTextureData* GetRawData() const { return &m_Data; }

		virtual const Vector3 GetSize() const override { return Vector3((float)m_Data.Extent.width, (float)m_Data.Extent.height, (float)m_Data.Extent.depth); }

		static Ref<VulkanTexture> Create(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		static Ref<VulkanTexture> Create(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		static Ref<VulkanTexture> Create(const char* filePath);

		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		static void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

	private:

		VulkanTextureData m_Data;
	};
}
