#pragma once

#include <Backends/Vulkan/VulkanCommon.h>
#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/Buffer.h>

namespace Spike {

	namespace VulkanUtils {

		VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
		VkCommandBufferAllocateInfo CommandBufferAllocInfo(VkCommandPool pool, uint32_t count = 1);

		VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);
		VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

		VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
		VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
		VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
		VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
		VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);
		VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachments, uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* depthAttachment);
		VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);

		VkFormat TextureFormatToVulkan(ETextureFormat format);
		VkImageUsageFlags TextureUsageFlagsToVulkan(ETextureUsageFlags flags);
		VkImageLayout GPUAccessToVulkanLayout(EGPUAccessFlags flags);
		VkAccessFlags2 GPUAccessToVulkanAccess(EGPUAccessFlags flags);
		VkPipelineStageFlags2 GPUAccessToVulkanStage(EGPUAccessFlags flags);
		VkBufferUsageFlags BufferUsageToVulkan(EBufferUsageFlags flags);
		VmaMemoryUsage BufferMemUsageToVulkan(EBufferMemUsage usage);
		VkDescriptorType BindingTypeToVulkan(EShaderResourceType type);
		VkFrontFace FrontFaceToVulkan(EFrontFace face);
		VkFilter SamplerFilterToVulkan(ESamplerFilter filter);
		VkSamplerMipmapMode SamplerMipMapModeToVulkan(ESamplerFilter filter);
		VkSamplerAddressMode SamplerAddressToVulkan(ESamplerAddress address);
		VkSamplerReductionMode SamplerReductionToVulkan(ESamplerReduction reduction);

		VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);
		VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
		VkImageCreateInfo CubeImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, uint32_t size);
		VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry);
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();

		void BarrierImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout);
		void BarrierImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout,
			VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);
		//void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

		void BarrierBuffer(VkCommandBuffer cmd, VkBuffer buffer, size_t size, size_t offset, VkAccessFlags2 srcAccess, VkAccessFlags2 dstAccess,
			VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);
	}
}
