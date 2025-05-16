#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>

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
		VkImageCreateInfo CubeImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
		VkImageViewCreateInfo ImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
		VkImageViewCreateInfo CubeImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

		VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);
		VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachments, uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* depthAttachment);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();

		VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view, VkImageLayout layout);
	}
}
