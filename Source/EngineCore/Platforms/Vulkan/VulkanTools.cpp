#include <Platforms/Vulkan/VulkanTools.h>


VkCommandPoolCreateInfo Spike::VulkanTools::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;

	return info;
}

VkCommandBufferAllocateInfo Spike::VulkanTools::CommandBufferAllocInfo(VkCommandPool pool, uint32_t count) {

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;

	info.commandPool = pool;
	info.commandBufferCount = count;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	return info;
}

VkFenceCreateInfo Spike::VulkanTools::FenceCreateInfo(VkFenceCreateFlags flags) {

	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = flags;

	return info;
}

VkSemaphoreCreateInfo Spike::VulkanTools::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags) {

	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = flags;

	return info;
}

VkCommandBufferBeginInfo Spike::VulkanTools::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags) {

	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;

	info.pInheritanceInfo = nullptr;
	info.flags = flags;

	return info;
}

VkImageSubresourceRange Spike::VulkanTools::ImageSubresourceRange(VkImageAspectFlags aspectMask) {

	VkImageSubresourceRange subImage{};
	subImage.aspectMask = aspectMask;
	subImage.baseMipLevel = 0;
	subImage.levelCount = VK_REMAINING_MIP_LEVELS;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return subImage;
}

VkSemaphoreSubmitInfo Spike::VulkanTools::SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {

	VkSemaphoreSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.semaphore = semaphore;
	submitInfo.stageMask = stageMask;
	submitInfo.deviceIndex = 0;
	submitInfo.value = 1;

	return submitInfo;
}

VkCommandBufferSubmitInfo Spike::VulkanTools::CommandBufferSubmitInfo(VkCommandBuffer cmd) {

	VkCommandBufferSubmitInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	info.pNext = nullptr;
	info.commandBuffer = cmd;
	info.deviceMask = 0;

	return info;
}

VkSubmitInfo2 Spike::VulkanTools::SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo) {

	VkSubmitInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.pNext = nullptr;

	info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
	info.pWaitSemaphoreInfos = waitSemaphoreInfo;

	info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
	info.pSignalSemaphoreInfos = signalSemaphoreInfo;

	info.commandBufferInfoCount = 1;
	info.pCommandBufferInfos = cmd;

	return info;
}

VkImageCreateInfo Spike::VulkanTools::ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.imageType = VK_IMAGE_TYPE_2D;

	info.format = format;
	info.extent = extent;

	info.mipLevels = 1;
	info.arrayLayers = 1;

	info.samples = VK_SAMPLE_COUNT_1_BIT;

	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	return info;
}

VkImageCreateInfo Spike::VulkanTools::CubeImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, uint32_t size) {

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.imageType = VK_IMAGE_TYPE_2D;

	info.format = format;
	info.extent = {size, size, 1};

	info.mipLevels = 1;
	info.arrayLayers = 6;

	info.samples = VK_SAMPLE_COUNT_1_BIT;

	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	return info;
}

VkImageViewCreateInfo Spike::VulkanTools::ImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;

	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = image;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = aspectFlags;

	return info;
}

VkImageViewCreateInfo Spike::VulkanTools::CubeImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;

	info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	info.image = image;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 6;
	info.subresourceRange.aspectMask = aspectFlags;

	return info;
}

VkRenderingAttachmentInfo Spike::VulkanTools::AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout) {

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.pNext = nullptr;

	colorAttachment.imageView = view;
	colorAttachment.imageLayout = layout;
	colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	if (clear) {
		colorAttachment.clearValue = *clear;
	}

	return colorAttachment;
}

VkRenderingInfo Spike::VulkanTools::RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachments, uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* depthAttachment) {

	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.pNext = nullptr;

	renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, renderExtent };
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = colorAttachmentCount;
	renderInfo.pColorAttachments = colorAttachments;
	renderInfo.pDepthAttachment = depthAttachment;
	renderInfo.pStencilAttachment = nullptr;

	return renderInfo;
}

VkPipelineShaderStageCreateInfo Spike::VulkanTools::PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry) {

	VkPipelineShaderStageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.stage = stage;
	info.module = shaderModule;
	info.pName = entry;

	return info;
}

VkPipelineLayoutCreateInfo Spike::VulkanTools::PipelineLayoutCreateInfo() {

	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = 0;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;

	return info;
}

VkRenderingAttachmentInfo Spike::VulkanTools::DepthAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout) {

	VkRenderingAttachmentInfo depthAttachment{};
	depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachment.pNext = nullptr;

	depthAttachment.imageView = view;
	depthAttachment.imageLayout = layout;
	depthAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	
	if (clear) {
		depthAttachment.clearValue = *clear;
	}

	return depthAttachment;
}

void Spike::VulkanTools::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask) {

	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext = nullptr;

	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = srcAccessMask;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = dstAccessMask;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = imageAspectFlags;
	imageBarrier.subresourceRange = VulkanTools::ImageSubresourceRange(aspectMask);
	imageBarrier.image = image;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

void Spike::VulkanTools::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout) {

	TransitionImage(cmd, image, imageAspectFlags, currentLayout, newLayout, VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT);
}

void Spike::VulkanTools::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {

	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}

void Spike::VulkanTools::GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize) {

	int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;
	for (int mip = 0; mip < mipLevels; mip++) {

		VkExtent2D halfSize = imageSize;
		halfSize.width /= 2;
		halfSize.height /= 2;

		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr };

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange = VulkanTools::ImageSubresourceRange(aspectMask);
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = mip;
		imageBarrier.image = image;

		VkDependencyInfo depInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);

		if (mip < mipLevels - 1) {
			VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

			blitRegion.srcOffsets[1].x = imageSize.width;
			blitRegion.srcOffsets[1].y = imageSize.height;
			blitRegion.srcOffsets[1].z = 1;

			blitRegion.dstOffsets[1].x = halfSize.width;
			blitRegion.dstOffsets[1].y = halfSize.height;
			blitRegion.dstOffsets[1].z = 1;

			blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.srcSubresource.baseArrayLayer = 0;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcSubresource.mipLevel = mip;

			blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.dstSubresource.baseArrayLayer = 0;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstSubresource.mipLevel = mip + 1;

			VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
			blitInfo.dstImage = image;
			blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			blitInfo.srcImage = image;
			blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			blitInfo.filter = VK_FILTER_LINEAR;
			blitInfo.regionCount = 1;
			blitInfo.pRegions = &blitRegion;

			vkCmdBlitImage2(cmd, &blitInfo);

			imageSize = halfSize;
		}
	}

	// transition all mip levels into the final read_only layout
	TransitionImage(cmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Spike::VulkanTools::TransitionImageCube(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout) {

	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext = nullptr;

	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = imageAspectFlags;
	imageBarrier.subresourceRange = VulkanTools::ImageSubresourceRange(aspectMask);
	imageBarrier.image = image;

	//imageBarrier.subresourceRange.baseArrayLayer = 0;
	//imageBarrier.subresourceRange.layerCount = 6;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

void Spike::VulkanTools::CopyImageToImageCube(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {

	std::vector<VkImageBlit2> blitRegion(6);

	for (int i = 0; i < 6; i++) {

		blitRegion[i] = { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

		blitRegion[i].srcOffsets[1].x = srcSize.width;
		blitRegion[i].srcOffsets[1].y = srcSize.height;
		blitRegion[i].srcOffsets[1].z = 1;

		blitRegion[i].dstOffsets[1].x = dstSize.width;
		blitRegion[i].dstOffsets[1].y = dstSize.height;
		blitRegion[i].dstOffsets[1].z = 1;

		blitRegion[i].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion[i].srcSubresource.baseArrayLayer = i;
		blitRegion[i].srcSubresource.layerCount = 1;
		blitRegion[i].srcSubresource.mipLevel = 0;

		blitRegion[i].dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion[i].dstSubresource.baseArrayLayer = i;
		blitRegion[i].dstSubresource.layerCount = 1;
		blitRegion[i].dstSubresource.mipLevel = 0;
	}

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 6;
	blitInfo.pRegions = blitRegion.data();

	vkCmdBlitImage2(cmd, &blitInfo);
}

void Spike::VulkanTools::TransitionBuffer(VkCommandBuffer cmd, VkBuffer buffer, size_t size, size_t offset, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcPipelineStage, VkPipelineStageFlags2 dstPipelineStage) {

	VkBufferMemoryBarrier2 barrier = {

		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.srcStageMask = srcPipelineStage,
		.srcAccessMask = srcAccessMask,
		.dstStageMask = dstPipelineStage,
		.dstAccessMask = dstAccessMask,
		.buffer = buffer,
		.offset = offset,
		.size = size
	};

	VkDependencyInfo depInfo = {

		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.bufferMemoryBarrierCount = 1,
		.pBufferMemoryBarriers = &barrier
	};

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

void Spike::VulkanTools::FillBuffer(VkCommandBuffer cmd, VkBuffer buffer, size_t size, size_t offset, uint32_t value, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcPipelineStage, VkPipelineStageFlags2 dstPipelineStage) {

	TransitionBuffer(cmd, buffer, size, offset, srcAccessMask, VK_ACCESS_2_TRANSFER_WRITE_BIT, srcPipelineStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

	vkCmdFillBuffer(cmd, buffer, offset, size, value);

	TransitionBuffer(cmd, buffer, size, offset, VK_ACCESS_2_TRANSFER_WRITE_BIT, dstAccessMask, VK_PIPELINE_STAGE_2_TRANSFER_BIT, dstPipelineStage);
}

VkFormat Spike::VulkanTools::TextureFormatToVulkan(ETextureFormat format) {

	switch (format)
	{
	case Spike::EFormatNone:
		return VK_FORMAT_UNDEFINED;
		break;
	case Spike::EFormatRGBA8Unorm:
		return VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case Spike::EFormatRGBA16SFloat:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
		break;
	case Spike::EFormatRGBA32SFloat:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case Spike::EFormatD32SFloat:
		return VK_FORMAT_D32_SFLOAT;
		break;
	default:
		return VK_FORMAT_UNDEFINED;
		break;
	}
}

VkImageUsageFlags Spike::VulkanTools::TextureUsageFlagsToVulkan(ETextureUsageFlags usageFlags) {

	VkImageUsageFlags outFlags = 0;

	if (usageFlags & EUsageFlagSampled) outFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (usageFlags & EUsageFlagColorAttachment) outFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (usageFlags & EUsageFlagDepthAtachment) outFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (usageFlags & EUsageFlagStorage) outFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (usageFlags & EUsageFlagTransferDst) outFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (usageFlags & EUsageFlagTransferSrc) outFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	return outFlags;
}

uint32_t Spike::VulkanTools::GetComputeGroupCount(uint32_t threadCount, uint32_t groupSize) {

	return (threadCount + groupSize - 1) / groupSize;
}