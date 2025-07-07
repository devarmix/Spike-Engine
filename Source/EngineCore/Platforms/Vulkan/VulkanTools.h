#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Platforms/Vulkan/VulkanShader.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/TextureBase.h>

namespace Spike {

	namespace VulkanTools {

		VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
		VkCommandBufferAllocateInfo CommandBufferAllocInfo(VkCommandPool pool, uint32_t count = 1);

		VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);
		VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

		VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

		VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

		VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
		VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
		VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);

		VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
		VkImageCreateInfo CubeImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, uint32_t size);
		VkImageViewCreateInfo ImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
		VkImageViewCreateInfo CubeImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

		VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);
		VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachments, uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* depthAttachment);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();

		VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);

		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout);
		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask);
		void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		void GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

		void TransitionImageCube(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout);
		void CopyImageToImageCube(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

		void TransitionBuffer(VkCommandBuffer cmd, VkBuffer buffer, size_t size, size_t offset, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, 
			VkPipelineStageFlags2 srcPipelineStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VkPipelineStageFlags2 dstPipelineStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

		void FillBuffer(VkCommandBuffer cmd, VkBuffer buffer, size_t size, size_t offset, uint32_t value, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask,
			VkPipelineStageFlags2 srcPipelineStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VkPipelineStageFlags2 dstPipelineStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

		VkFormat TextureFormatToVulkan(ETextureFormat format);
		VkImageUsageFlags TextureUsageFlagsToVulkan(ETextureUsageFlags usageFlags);

		uint32_t GetComputeGroupCount(uint32_t threadCount, uint32_t groupSize);
	}
}
