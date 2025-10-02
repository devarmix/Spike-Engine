#include <Backends/Vulkan/VulkanUtils.h>

VkCommandPoolCreateInfo Spike::VulkanUtils::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;

	return info;
}

VkCommandBufferAllocateInfo Spike::VulkanUtils::CommandBufferAllocInfo(VkCommandPool pool, uint32_t count) {

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;

	info.commandPool = pool;
	info.commandBufferCount = count;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	return info;
}

VkFenceCreateInfo Spike::VulkanUtils::FenceCreateInfo(VkFenceCreateFlags flags) {

	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = flags;

	return info;
}

VkSemaphoreCreateInfo Spike::VulkanUtils::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags) {

	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = flags;

	return info;
}

VkCommandBufferBeginInfo Spike::VulkanUtils::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags) {

	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;

	info.pInheritanceInfo = nullptr;
	info.flags = flags;

	return info;
}

VkSemaphoreSubmitInfo Spike::VulkanUtils::SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {

	VkSemaphoreSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.semaphore = semaphore;
	submitInfo.stageMask = stageMask;
	submitInfo.deviceIndex = 0;
	submitInfo.value = 1;

	return submitInfo;
}

VkCommandBufferSubmitInfo Spike::VulkanUtils::CommandBufferSubmitInfo(VkCommandBuffer cmd) {

	VkCommandBufferSubmitInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	info.pNext = nullptr;
	info.commandBuffer = cmd;
	info.deviceMask = 0;

	return info;
}

VkSubmitInfo2 Spike::VulkanUtils::SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo) {

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


VkRenderingAttachmentInfo Spike::VulkanUtils::AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout) {

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

VkRenderingInfo Spike::VulkanUtils::RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachments, uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* depthAttachment) {

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

VkRenderingAttachmentInfo Spike::VulkanUtils::DepthAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout) {

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

VkFormat Spike::VulkanUtils::TextureFormatToVulkan(ETextureFormat format) {

	switch (format)
	{
	case Spike::ETextureFormat::ERGBA8U:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case Spike::ETextureFormat::EBGRA8U:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case Spike::ETextureFormat::ERGBA16F:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case Spike::ETextureFormat::ERGBA32F:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case Spike::ETextureFormat::ED32F:
		return VK_FORMAT_D32_SFLOAT;
	case Spike::ETextureFormat::ER32F:
		return VK_FORMAT_R32_SFLOAT;
	case Spike::ETextureFormat::ERG16F:
		return VK_FORMAT_R16G16_SFLOAT;
	case Spike::ETextureFormat::ERG8U:
		return VK_FORMAT_R8G8_UNORM;
	case Spike::ETextureFormat::ER8U:
		return VK_FORMAT_R8_UNORM;
	case Spike::ETextureFormat::ERGBABC3:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case Spike::ETextureFormat::ERGBABC6:
		return VK_FORMAT_BC6H_SFLOAT_BLOCK;
	case Spike::ETextureFormat::ERGBC5:
		return VK_FORMAT_BC5_UNORM_BLOCK;
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

VkImageUsageFlags Spike::VulkanUtils::TextureUsageFlagsToVulkan(ETextureUsageFlags flags) {

	VkImageUsageFlags outFlags = 0;

	if (EnumHasAllFlags(flags, ETextureUsageFlags::ESampled)) outFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (EnumHasAllFlags(flags, ETextureUsageFlags::EStorage)) outFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (EnumHasAllFlags(flags, ETextureUsageFlags::EColorTarget)) outFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (EnumHasAllFlags(flags, ETextureUsageFlags::EDepthTarget)) outFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (EnumHasAllFlags(flags, ETextureUsageFlags::ECopySrc)) outFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (EnumHasAllFlags(flags, ETextureUsageFlags::ECopyDst)) outFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	return outFlags;
}

VkImageLayout Spike::VulkanUtils::GPUAccessToVulkanLayout(EGPUAccessFlags flags) {

	if (EnumHasAnyFlags(flags, EGPUAccessFlags::EUAVCompute | EGPUAccessFlags::EUAVGraphics)) return VK_IMAGE_LAYOUT_GENERAL;
	else if (EnumHasAnyFlags(flags, EGPUAccessFlags::ESRVCompute | EGPUAccessFlags::ESRVGraphics)) return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	else if (EnumHasAllFlags(flags, EGPUAccessFlags::ECopySrc)) return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	else if (EnumHasAllFlags(flags, EGPUAccessFlags::ECopyDst)) return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	else if (EnumHasAllFlags(flags, EGPUAccessFlags::EColorTarget)) return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	else if (EnumHasAllFlags(flags, EGPUAccessFlags::EDepthTarget)) return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	else return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags2 Spike::VulkanUtils::GPUAccessToVulkanAccess(EGPUAccessFlags flags) {

	VkAccessFlags2 outFlags = 0;

	if (EnumHasAnyFlags(flags, EGPUAccessFlags::EUAVCompute | EGPUAccessFlags::EUAVGraphics)) outFlags |= VK_ACCESS_2_SHADER_WRITE_BIT;
	if (EnumHasAnyFlags(flags, EGPUAccessFlags::ESRVCompute | EGPUAccessFlags::ESRVGraphics)) outFlags |= VK_ACCESS_2_SHADER_READ_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::ECopySrc)) outFlags |= VK_ACCESS_2_TRANSFER_READ_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::ECopyDst)) outFlags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::EColorTarget)) outFlags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::EDepthTarget)) outFlags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::EIndirectArgs)) outFlags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

	return outFlags;
}

VkPipelineStageFlags2 Spike::VulkanUtils::GPUAccessToVulkanStage(EGPUAccessFlags flags) {

	VkPipelineStageFlags2 outFlags = 0;

	if (EnumHasAnyFlags(flags, EGPUAccessFlags::ESRVCompute | EGPUAccessFlags::EUAVCompute)) outFlags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	if (EnumHasAnyFlags(flags, EGPUAccessFlags::ESRVGraphics | EGPUAccessFlags::EUAVGraphics)) outFlags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::EIndirectArgs)) outFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
	if (EnumHasAnyFlags(flags, EGPUAccessFlags::ECopySrc | EGPUAccessFlags::ECopyDst)) outFlags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::EColorTarget)) outFlags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	if (EnumHasAllFlags(flags, EGPUAccessFlags::EDepthTarget)) outFlags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

	return outFlags;
}

VkBufferUsageFlags Spike::VulkanUtils::BufferUsageToVulkan(EBufferUsageFlags flags) {

	VkBufferUsageFlags outFlags = 0;

	if (EnumHasAllFlags(flags, EBufferUsageFlags::EConstant)) outFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (EnumHasAllFlags(flags, EBufferUsageFlags::EStorage)) outFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (EnumHasAllFlags(flags, EBufferUsageFlags::ECopyDst)) outFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (EnumHasAllFlags(flags, EBufferUsageFlags::ECopySrc)) outFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (EnumHasAllFlags(flags, EBufferUsageFlags::EIndirect)) outFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if (EnumHasAllFlags(flags, EBufferUsageFlags::EAddressable)) outFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	if (EnumHasAllFlags(flags, EBufferUsageFlags::EIndex)) outFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	return outFlags;
}

VmaMemoryUsage Spike::VulkanUtils::BufferMemUsageToVulkan(EBufferMemUsage usage) {

	switch (usage)
	{
	case Spike::EBufferMemUsage::ECPUToGPU:
		return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case Spike::EBufferMemUsage::ECPUOnly:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	case Spike::EBufferMemUsage::EGPUOnly:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	default:
		return VMA_MEMORY_USAGE_UNKNOWN;
	}
}

VkDescriptorType Spike::VulkanUtils::BindingTypeToVulkan(EShaderResourceType type) {

	switch (type)
	{
	case Spike::EShaderResourceType::ETextureSRV:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case Spike::EShaderResourceType::ETextureUAV:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case Spike::EShaderResourceType::EBufferSRV:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case Spike::EShaderResourceType::EBufferUAV:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case Spike::EShaderResourceType::EConstantBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case Spike::EShaderResourceType::ESampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	default:
		ENGINE_ERROR("Unspecified binding type! Undefined behaviour");
		return (VkDescriptorType)0;
	}
}

VkFrontFace Spike::VulkanUtils::FrontFaceToVulkan(EFrontFace face) {

	switch (face)
	{
	case Spike::EFrontFace::EClockWise:
		return VK_FRONT_FACE_CLOCKWISE;
	case Spike::EFrontFace::ECounterClockWise:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	default:
		ENGINE_ERROR("Unspecified front face type! Undefined behaviour");
		return (VkFrontFace)0;
	}
}

VkFilter Spike::VulkanUtils::SamplerFilterToVulkan(ESamplerFilter filter) {

	switch (filter)
	{
	case Spike::ESamplerFilter::EPoint:
		return VK_FILTER_NEAREST;
	case Spike::ESamplerFilter::EBilinear:
		return VK_FILTER_LINEAR;
	case Spike::ESamplerFilter::ETrilinear:
		return VK_FILTER_LINEAR;
	default:
		ENGINE_ERROR("Unspecified sampler filter! Undefined behaviour");
		return (VkFilter)0;
	}
}

VkSamplerMipmapMode Spike::VulkanUtils::SamplerMipMapModeToVulkan(ESamplerFilter filter) {

	switch (filter)
	{
	case Spike::ESamplerFilter::EPoint:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case Spike::ESamplerFilter::EBilinear:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case Spike::ESamplerFilter::ETrilinear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		ENGINE_ERROR("Unspecified sampler filter! Undefined behaviour");
		return (VkSamplerMipmapMode)0;
	}
}

VkSamplerAddressMode Spike::VulkanUtils::SamplerAddressToVulkan(ESamplerAddress address) {

	switch (address)
	{
	case Spike::ESamplerAddress::ERepeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case Spike::ESamplerAddress::EClamp:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case Spike::ESamplerAddress::EMirror:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	default:
		ENGINE_ERROR("Unspecified sampler address! Undefined behaviour");
		return (VkSamplerAddressMode)0;
	}
}

VkSamplerReductionMode Spike::VulkanUtils::SamplerReductionToVulkan(ESamplerReduction reduction) {

	switch (reduction)
	{
	case Spike::ESamplerReduction::EMinimum:
		return VK_SAMPLER_REDUCTION_MODE_MIN;
	case Spike::ESamplerReduction::EMaximum:
		return VK_SAMPLER_REDUCTION_MODE_MAX;
	default:
		return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
	}
}

VkImageSubresourceRange Spike::VulkanUtils::ImageSubresourceRange(VkImageAspectFlags aspectMask) {

	VkImageSubresourceRange subImage{};
	subImage.aspectMask = aspectMask;
	subImage.baseMipLevel = 0;
	subImage.levelCount = VK_REMAINING_MIP_LEVELS;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return subImage;
}

VkImageCreateInfo Spike::VulkanUtils::ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {

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

VkImageCreateInfo Spike::VulkanUtils::CubeImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, uint32_t size) {

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.imageType = VK_IMAGE_TYPE_2D;

	info.format = format;
	info.extent = { size, size, 1 };

	info.mipLevels = 1;
	info.arrayLayers = 6;

	info.samples = VK_SAMPLE_COUNT_1_BIT;

	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	return info;
}

VkImageViewCreateInfo Spike::VulkanUtils::ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {

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

VkPipelineShaderStageCreateInfo Spike::VulkanUtils::PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry) {

	VkPipelineShaderStageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.stage = stage;
	info.module = shaderModule;
	info.pName = entry;

	return info;
}

VkPipelineLayoutCreateInfo Spike::VulkanUtils::PipelineLayoutCreateInfo() {

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

void Spike::VulkanUtils::BarrierImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout,
	VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage) 
{

	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext = nullptr;

	imageBarrier.srcStageMask = srcStage;
	imageBarrier.srcAccessMask = srcAccessMask;
	imageBarrier.dstStageMask = dstStage;
	imageBarrier.dstAccessMask = dstAccessMask;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = imageAspectFlags;
	imageBarrier.subresourceRange = VulkanUtils::ImageSubresourceRange(aspectMask);
	imageBarrier.image = image;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

void Spike::VulkanUtils::BarrierImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout) {

	BarrierImage(cmd, image, imageAspectFlags, currentLayout, newLayout, VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
	    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
}

void Spike::VulkanUtils::BarrierBuffer(VkCommandBuffer cmd, VkBuffer buffer, size_t size, size_t offset, VkAccessFlags2 srcAccess, VkAccessFlags2 dstAccess,
	VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage)
{
	VkBufferMemoryBarrier2 barrier = {

		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.srcStageMask = srcStage,
		.srcAccessMask = srcAccess,
		.dstStageMask = dstStage,
		.dstAccessMask = dstAccess,
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