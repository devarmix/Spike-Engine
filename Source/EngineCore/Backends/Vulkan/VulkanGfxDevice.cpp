#include <Backends/Vulkan/VulkanGfxDevice.h>
#include <Backends/Vulkan/VulkanUtils.h>
#include <Backends/Vulkan/VulkanPipeline.h>
#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	VulkanRHIDevice::VulkanRHIDevice(Window* window, bool useImgui) {

		m_Device.Init(window, true);
		m_Swapchain.Init(m_Device, window->GetWidth(), window->GetHeight());

		// init sync structures
		{
			VkFenceCreateInfo fenceCreateInfo = VulkanUtils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
			VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanUtils::SemaphoreCreateInfo();

			for (int i = 0; i < 2; i++) {

				VK_CHECK(vkCreateFence(m_Device.Device, &fenceCreateInfo, nullptr, &m_SyncObjects[i].RenderFence));
				VK_CHECK(vkCreateSemaphore(m_Device.Device, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].SwapchainSemaphore));
				VK_CHECK(vkCreateSemaphore(m_Device.Device, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].RenderSemaphore));
			}

			// immediate fence
			VK_CHECK(vkCreateFence(m_Device.Device, &fenceCreateInfo, nullptr, &m_ImmFence));

			m_ImmCmd = nullptr;
		}

		// init pools
		{
			std::array<VkDescriptorPoolSize, 2> poolSizes{

				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 500},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 500}
			};

			VkDescriptorPoolCreateInfo PoolInfo{};
			PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			PoolInfo.poolSizeCount = (uint32_t)poolSizes.size();
			PoolInfo.pPoolSizes = poolSizes.data();
			PoolInfo.maxSets = 1;
			PoolInfo.pNext = nullptr;

			VK_CHECK(vkCreateDescriptorPool(m_Device.Device, &PoolInfo, nullptr, &m_BindlessPool));
		}
		{
			std::array<VkDescriptorPoolSize, 5> poolSizes{

				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 500},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 500},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 500},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 500},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 500}
			};

			VkDescriptorPoolCreateInfo PoolInfo{};
			PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			PoolInfo.poolSizeCount = (uint32_t)poolSizes.size();
			PoolInfo.pPoolSizes = poolSizes.data();
			PoolInfo.maxSets = 500;
			PoolInfo.pNext = nullptr;

			VK_CHECK(vkCreateDescriptorPool(m_Device.Device, &PoolInfo, nullptr, &m_GlobalSetPool));
		}

		m_UsingImGui = useImgui;
		m_ImGuiShader = nullptr;
	}

	VulkanRHIDevice::~VulkanRHIDevice() {

		for (int i = 0; i < 2; i++) {

			vkDestroyFence(m_Device.Device, m_SyncObjects[i].RenderFence, nullptr);
			vkDestroySemaphore(m_Device.Device, m_SyncObjects[i].RenderSemaphore, nullptr);
			vkDestroySemaphore(m_Device.Device, m_SyncObjects[i].SwapchainSemaphore, nullptr);
		}

		if (m_UsingImGui) {
			m_GuiTextureManager.Cleanup();

			for (int i = 0; i < 2; i++) {
				DestroyBufferRHI((RHIData)m_ImGuiDrawObjects[i].IdxBuffer);
				DestroyBufferRHI((RHIData)m_ImGuiDrawObjects[i].VtxBuffer);
			}
		}

		vkDestroyFence(m_Device.Device, m_ImmFence, nullptr);

		if (m_ImmCmd) {
			m_ImmCmd->ReleaseRHI();
			delete m_ImmCmd;
		}

		vkDestroyDescriptorPool(m_Device.Device, m_BindlessPool, nullptr);
		vkDestroyDescriptorPool(m_Device.Device, m_GlobalSetPool, nullptr);

		m_Swapchain.Destroy(m_Device);
		m_Device.Destroy();
	}

	RHIData VulkanRHIDevice::CreateTexture2DRHI(const Texture2DDesc& desc) {

		VulkanRHITexture* tex = new VulkanRHITexture();

		VkFormat vkFormat = VulkanUtils::TextureFormatToVulkan(desc.Format);
		VkImageUsageFlags vkFlags = VulkanUtils::TextureUsageFlagsToVulkan(desc.UsageFlags);

		VkImageCreateInfo imgInfo = VulkanUtils::ImageCreateInfo(vkFormat, vkFlags, { desc.Width, desc.Height, 1 });
		imgInfo.mipLevels = desc.NumMips;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK(vmaCreateImage(m_Device.Allocator, &imgInfo, &allocInfo, &tex->Image, &tex->Allocation, nullptr));
		return (RHIData)tex;
	}

	void VulkanRHIDevice::CopyDataToTexture(void* src, size_t srcOffset, RHITexture* dst, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess, 
		const std::vector<SubResourceCopyRegion>& regions, size_t copySize) 
	{
		VulkanRHITexture* vkTex = (VulkanRHITexture*)dst->GetRHIData();

		BufferDesc uploadDesc{};
		uploadDesc.Size = copySize;
		uploadDesc.UsageFlags = EBufferUsageFlags::ECopySrc;
		uploadDesc.MemUsage = EBufferMemUsage::ECPUToGPU;

		VulkanRHIBuffer* uploadBuff = (VulkanRHIBuffer*)CreateBufferRHI(uploadDesc);
		memcpy(uploadBuff->AllocationInfo.pMappedData, (uint8_t*)src + srcOffset, copySize);

		ImmediateSubmit([&](RHICommandBuffer* cmd) {

			VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
			BarrierTexture(cmd, dst, lastAccess, EGPUAccessFlags::ECopyDst);

			std::vector<VkBufferImageCopy> vkRegions{};
			for (auto& region : regions) {

				VkBufferImageCopy vkRegion = {};
				vkRegion.bufferOffset = region.DataOffset;
				vkRegion.bufferRowLength = 0;
				vkRegion.bufferImageHeight = 0;

				vkRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				vkRegion.imageSubresource.mipLevel = region.MipLevel;
				vkRegion.imageSubresource.baseArrayLayer = region.ArrayLayer;
				vkRegion.imageSubresource.layerCount = 1;
				vkRegion.imageExtent = { (dst->GetSizeXYZ().x >> region.MipLevel), (dst->GetSizeXYZ().y >> region.MipLevel), 1 };
				vkRegion.imageOffset = { 0, 0, 0 };

				vkRegions.push_back(vkRegion);
			}

			vkCmdCopyBufferToImage(vkCmd->Cmd, uploadBuff->Buffer, vkTex->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkRegions.size(), vkRegions.data());
			BarrierTexture(cmd, dst, EGPUAccessFlags::ECopyDst, newAccess);
			});

		DestroyBufferRHI((RHIData)uploadBuff);
	}

	void VulkanRHIDevice::MipMapTexture2D(RHICommandBuffer* cmd, RHITexture2D* tex, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess, uint32_t numMips) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		VulkanRHITexture* vkTex = (VulkanRHITexture*)tex->GetRHIData();

		VkExtent2D imageSize = { tex->GetSizeXYZ().x, tex->GetSizeXYZ().y };
		BarrierTexture(cmd, tex, lastAccess, EGPUAccessFlags::ECopyDst);

		for (uint32_t mip = 0; mip < numMips; mip++) {

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
			imageBarrier.subresourceRange = VulkanUtils::ImageSubresourceRange(aspectMask);
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseMipLevel = mip;
			imageBarrier.image = vkTex->Image;

			VkDependencyInfo depInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr };
			depInfo.imageMemoryBarrierCount = 1;
			depInfo.pImageMemoryBarriers = &imageBarrier;

			vkCmdPipelineBarrier2(vkCmd->Cmd, &depInfo);

			if (mip < numMips - 1) {
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
				blitInfo.dstImage = vkTex->Image;
				blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				blitInfo.srcImage = vkTex->Image;
				blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				blitInfo.filter = VK_FILTER_LINEAR;
				blitInfo.regionCount = 1;
				blitInfo.pRegions = &blitRegion;

				vkCmdBlitImage2(vkCmd->Cmd, &blitInfo);
				imageSize = halfSize;
			}
		}
		BarrierTexture(cmd, tex, EGPUAccessFlags::ECopySrc, newAccess);
	}

	void VulkanRHIDevice::BarrierTexture(RHICommandBuffer* cmd, RHITexture* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		VulkanRHITexture* vkTex = (VulkanRHITexture*)texture->GetRHIData();

		VkImageLayout vkLastLayout = VulkanUtils::GPUAccessToVulkanLayout(lastAccess);
		VkAccessFlags2 vkLastAccess = VulkanUtils::GPUAccessToVulkanAccess(lastAccess);
		VkPipelineStageFlags2 vkLastStage = VulkanUtils::GPUAccessToVulkanStage(lastAccess);

		VkImageLayout vkNewLayout = VulkanUtils::GPUAccessToVulkanLayout(newAccess);
		VkAccessFlags2 vkNewAccess = VulkanUtils::GPUAccessToVulkanAccess(newAccess);
		VkPipelineStageFlags2 vkNewStage = VulkanUtils::GPUAccessToVulkanStage(newAccess);

		VkImageAspectFlags aspect = texture->GetFormat() == ETextureFormat::ED32F ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		VulkanUtils::BarrierImage(vkCmd->Cmd, vkTex->Image, aspect, vkLastLayout, vkNewLayout, vkLastAccess, vkNewAccess, vkLastStage, vkNewStage);
	}

	void VulkanRHIDevice::DestroyTexture2DRHI(RHIData data) {

		VulkanRHITexture* vkTex = (VulkanRHITexture*)data;

		if (vkTex->Image) {
			vmaDestroyImage(m_Device.Allocator, vkTex->Image, vkTex->Allocation);
		}

		delete vkTex; 
	}

	RHIData VulkanRHIDevice::CreateCubeTextureRHI(const CubeTextureDesc& desc) {

		VulkanRHITexture* tex = new VulkanRHITexture();

		VkFormat vkFormat = VulkanUtils::TextureFormatToVulkan(desc.Format);
		VkImageUsageFlags vkFlags = VulkanUtils::TextureUsageFlagsToVulkan(desc.UsageFlags);

		VkImageCreateInfo imgInfo = VulkanUtils::CubeImageCreateInfo(vkFormat, vkFlags, desc.Size);
		imgInfo.mipLevels = desc.NumMips;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(m_Device.Allocator, &imgInfo, &allocInfo, &tex->Image, &tex->Allocation, nullptr));

		return (RHIData)tex;
	}

	void VulkanRHIDevice::DestroyCubeTextureRHI(RHIData data) {

		VulkanRHITexture* vkTex = (VulkanRHITexture*)data;

		if (vkTex->Image) {
			vmaDestroyImage(m_Device.Allocator, vkTex->Image, vkTex->Allocation);
		}

		delete vkTex;
	}

	void VulkanRHIDevice::CopyTexture(RHICommandBuffer* cmd, RHITexture* src, const TextureCopyRegion& srcRegion, RHITexture* dst, 
		const TextureCopyRegion& dstRegion, Vec2Uint copySize) 
	{
		VulkanRHITexture* vkSrc = (VulkanRHITexture*)src->GetRHIData();
		VulkanRHITexture* vkDst = (VulkanRHITexture*)dst->GetRHIData();
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		VkImageCopy2 copyRegion{};
		copyRegion.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = srcRegion.BaseArrayLayer;
		copyRegion.srcSubresource.mipLevel = srcRegion.MipLevel;
		copyRegion.srcSubresource.layerCount = srcRegion.LayerCount;
		copyRegion.srcOffset = { srcRegion.Offset.x, srcRegion.Offset.y, srcRegion.Offset.z };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = dstRegion.BaseArrayLayer;
		copyRegion.dstSubresource.mipLevel = dstRegion.MipLevel;
		copyRegion.dstSubresource.layerCount = dstRegion.LayerCount;
		copyRegion.dstOffset = { dstRegion.Offset.x, dstRegion.Offset.y, dstRegion.Offset.z };

		copyRegion.extent.width = copySize.x;
		copyRegion.extent.height = copySize.y;
		copyRegion.extent.depth = 1;

		VkCopyImageInfo2 copyInfo{};
		copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
		copyInfo.srcImage = vkSrc->Image;
		copyInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		copyInfo.dstImage = vkDst->Image;
		copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyInfo.regionCount = 1;
		copyInfo.pRegions = &copyRegion;

		vkCmdCopyImage2(vkCmd->Cmd, &copyInfo);
	}

	void VulkanRHIDevice::CopyFromTextureToCPU(RHICommandBuffer* cmd, RHITexture* src, SubResourceCopyRegion region, RHIBuffer* dst) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		VulkanRHITexture* vkTex = (VulkanRHITexture*)src->GetRHIData();
		VulkanRHIBuffer* vkBuff = (VulkanRHIBuffer*)dst->GetRHIData();

		VkBufferImageCopy vkRegion = {};
		vkRegion.bufferOffset = region.DataOffset;
		vkRegion.bufferRowLength = 0;
		vkRegion.bufferImageHeight = 0;

		vkRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkRegion.imageSubresource.mipLevel = region.MipLevel;
		vkRegion.imageSubresource.baseArrayLayer = region.ArrayLayer;
		vkRegion.imageSubresource.layerCount = 1;
		vkRegion.imageExtent = { (src->GetSizeXYZ().x >> region.MipLevel), (src->GetSizeXYZ().y >> region.MipLevel), 1 };
		vkRegion.imageOffset = { 0, 0, 0 };

		vkCmdCopyImageToBuffer(vkCmd->Cmd, vkTex->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkBuff->Buffer, 1, &vkRegion);
	}

	void VulkanRHIDevice::ClearTexture(RHICommandBuffer* cmd, RHITexture* tex, EGPUAccessFlags access, const Vec4& color) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		VulkanRHITexture* vkTex = (VulkanRHITexture*)tex->GetRHIData();

		VkImageSubresourceRange clearRange = VulkanUtils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		VkClearColorValue clearValue = { color.x, color.y, color.z, color.w };

		vkCmdClearColorImage(vkCmd->Cmd, vkTex->Image, VulkanUtils::GPUAccessToVulkanLayout(access), &clearValue, 1, &clearRange);
	}

	RHIData VulkanRHIDevice::CreateTextureViewRHI(const TextureViewDesc& desc) {

		VulkanRHITextureView* view = new VulkanRHITextureView();

		VkFormat vkFormat = VulkanUtils::TextureFormatToVulkan(desc.SourceTexture->GetFormat());
		VulkanRHITexture* vkTex = (VulkanRHITexture*)desc.SourceTexture->GetRHIData();
		VkImageAspectFlags aspect = vkFormat == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageViewCreateInfo viewInfo = VulkanUtils::ImageViewCreateInfo(vkFormat, vkTex->Image, aspect);
		if ((desc.Type == ETextureType::ECube) || 
			(desc.Type == ETextureType::ENone && desc.SourceTexture->GetTextureType() == ETextureType::ECube)) viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

		viewInfo.subresourceRange.levelCount = desc.NumMips;
		viewInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
		viewInfo.subresourceRange.baseMipLevel = desc.BaseMip;
		viewInfo.subresourceRange.layerCount = desc.NumArrayLayers;

		VK_CHECK(vkCreateImageView(m_Device.Device, &viewInfo, nullptr, &view->View));
		
		return (RHIData)view;
	}

	void VulkanRHIDevice::DestroyTextureViewRHI(RHIData data) {

		VulkanRHITextureView* vkView = (VulkanRHITextureView*)data;

		if (vkView->View) {
			vkDestroyImageView(m_Device.Device, vkView->View, nullptr);
		}

		delete vkView;
	}

	RHIData VulkanRHIDevice::CreateBufferRHI(const BufferDesc& desc) {

		VulkanRHIBuffer* buff = new VulkanRHIBuffer();

		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.flags = 0;
		bufferInfo.size = desc.Size;
		bufferInfo.usage = VulkanUtils::BufferUsageToVulkan(desc.UsageFlags);

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = VulkanUtils::BufferMemUsageToVulkan(desc.MemUsage);
		vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_CHECK(vmaCreateBuffer(m_Device.Allocator, &bufferInfo, &vmaAllocInfo, &buff->Buffer,
			&buff->Allocation, &buff->AllocationInfo));

		return (RHIData)buff;
	}

	void VulkanRHIDevice::DestroyBufferRHI(RHIData data) {
		VulkanRHIBuffer* vkBuff = (VulkanRHIBuffer*)data;

		if (vkBuff->Buffer) {
			vmaDestroyBuffer(m_Device.Allocator, vkBuff->Buffer, vkBuff->Allocation);
		}

		delete vkBuff;
	}

	void VulkanRHIDevice::CopyBuffer(RHICommandBuffer* cmd, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) {

		VulkanRHIBuffer* vkSrc = (VulkanRHIBuffer*)srcBuffer->GetRHIData();
		VulkanRHIBuffer* vkDst = (VulkanRHIBuffer*)dstBuffer->GetRHIData();
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		VkBufferCopy bufferCopy{};
		bufferCopy.dstOffset = dstOffset;
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.size = size;

		vkCmdCopyBuffer(vkCmd->Cmd, vkSrc->Buffer, vkDst->Buffer, 1, &bufferCopy);
	}

	void* VulkanRHIDevice::MapBufferMem(RHIBuffer* buffer) {

		VulkanRHIBuffer* vkBuff = (VulkanRHIBuffer*)buffer->GetRHIData();
		return vkBuff->AllocationInfo.pMappedData;
	}

	void VulkanRHIDevice::BarrierBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHIBuffer* vkBuff = (VulkanRHIBuffer*)buffer->GetRHIData();
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		VkAccessFlags2 vkLastAccess = VulkanUtils::GPUAccessToVulkanAccess(lastAccess);
		VkPipelineStageFlags2 vkLastStage = VulkanUtils::GPUAccessToVulkanStage(lastAccess);

		VkAccessFlags2 vkNewAccess = VulkanUtils::GPUAccessToVulkanAccess(newAccess);
		VkPipelineStageFlags2 vkNewStage = VulkanUtils::GPUAccessToVulkanStage(newAccess);

		VulkanUtils::BarrierBuffer(vkCmd->Cmd, vkBuff->Buffer, size, offset, vkLastAccess, vkNewAccess, vkLastStage, vkNewStage);
	}

	void VulkanRHIDevice::FillBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, uint32_t value, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHIBuffer* vkBuff = (VulkanRHIBuffer*)buffer->GetRHIData();
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		BarrierBuffer(cmd, buffer, size, offset, lastAccess, EGPUAccessFlags::ECopyDst);
		vkCmdFillBuffer(vkCmd->Cmd, vkBuff->Buffer, offset, size, value);
		BarrierBuffer(cmd, buffer, size, offset, EGPUAccessFlags::ECopyDst, newAccess);
	}

	uint64_t VulkanRHIDevice::GetBufferGPUAddress(RHIBuffer* buffer) {

		VulkanRHIBuffer* vkBuffer = (VulkanRHIBuffer*)buffer->GetRHIData();

		VkBufferDeviceAddressInfo vDeviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = vkBuffer->Buffer };
		return vkGetBufferDeviceAddress(m_Device.Device, &vDeviceAddressInfo);
	}

	RHIData VulkanRHIDevice::CreateBindingSetLayoutRHI(const BindingSetLayoutDesc& desc) {

		VulkanRHIBindingSetLayout* layout = new VulkanRHIBindingSetLayout();

		std::vector<VkDescriptorSetLayoutBinding> vkBindings;
		vkBindings.reserve(desc.Bindings.size());

		for (auto& b : desc.Bindings) {

			VkDescriptorSetLayoutBinding newBind{};
			newBind.binding = b.Slot;
			newBind.descriptorCount = b.Count;
			newBind.descriptorType = VulkanUtils::BindingTypeToVulkan(b.Type);
			newBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			newBind.pImmutableSamplers = nullptr;

			vkBindings.push_back(newBind);
		}

		VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.pBindings = vkBindings.data();
		info.bindingCount = (uint32_t)vkBindings.size();

		if (desc.UseDescriptorIndexing) {

			std::vector<VkDescriptorBindingFlags> layoutBindingFlags;
			layoutBindingFlags.resize(desc.Bindings.size());

			for (int i = 0; i < layoutBindingFlags.size(); i++) {
				layoutBindingFlags[i] = (vkBindings[i].descriptorCount > 1) ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0;
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsInfo{};
			layoutBindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			layoutBindingFlagsInfo.pNext = nullptr;
			layoutBindingFlagsInfo.pBindingFlags = layoutBindingFlags.data();
			layoutBindingFlagsInfo.bindingCount = (uint32_t)layoutBindingFlags.size();

			info.pNext = &layoutBindingFlagsInfo;
			info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			VK_CHECK(vkCreateDescriptorSetLayout(m_Device.Device, &info, nullptr, &layout->Layout));
		}
		else {

			info.pNext = nullptr;
			info.flags = 0;

			VK_CHECK(vkCreateDescriptorSetLayout(m_Device.Device, &info, nullptr, &layout->Layout));
		}

		return (RHIData)layout;
	}

	void VulkanRHIDevice::DestroyBindingSetLayoutRHI(RHIData data) {

		VulkanRHIBindingSetLayout* vkLayout = (VulkanRHIBindingSetLayout*)data;

		if (vkLayout->Layout) {
			vkDestroyDescriptorSetLayout(m_Device.Device, vkLayout->Layout, nullptr);
		}

		delete vkLayout;
	}

	RHIData VulkanRHIDevice::CreateBindingSetRHI(RHIBindingSetLayout* layout) {

		VulkanRHIBindingSet* set = new VulkanRHIBindingSet();
		VulkanRHIBindingSetLayout* vkLayout = (VulkanRHIBindingSetLayout*)layout->GetRHIData();

		set->AllocatedPool = layout->GetDesc().UseDescriptorIndexing ? m_BindlessPool : m_GlobalSetPool;

		VkDescriptorSetAllocateInfo SetInfo{};
		SetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetInfo.pNext = nullptr;
		SetInfo.descriptorPool = set->AllocatedPool;
		SetInfo.pSetLayouts = &vkLayout->Layout;
		SetInfo.descriptorSetCount = 1;

		VK_CHECK(vkAllocateDescriptorSets(m_Device.Device, &SetInfo, &set->Set));

		return (RHIData)set;
	}

	void VulkanRHIDevice::DestroyBindingSetRHI(RHIData data) {

		VulkanRHIBindingSet* vkSet = (VulkanRHIBindingSet*)data;

		if (vkSet->Set) {
			vkFreeDescriptorSets(m_Device.Device, vkSet->AllocatedPool, 1, &vkSet->Set);
		}

		delete vkSet;
	}

	RHIData VulkanRHIDevice::CreateShaderRHI(const ShaderDesc& desc, const ShaderCompiler::BinaryShader& binaryShader, const std::vector<RHIBindingSetLayout*>& layouts) {
		ENGINE_WARN(desc.Name);

		VulkanRHIShader* shader = new VulkanRHIShader();

		VkShaderModule vertexModule = nullptr;
		VkShaderModule pixelModule = nullptr;
		VkShaderModule computeModule = nullptr;

		VkPushConstantRange pushRange{};
		pushRange.offset = 0;
		pushRange.size = binaryShader.ShaderData.PushDataSize;
		pushRange.stageFlags = 0;

		if (desc.Type == EShaderType::EVertex || desc.Type == EShaderType::EGraphics) {

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.codeSize = binaryShader.VertexRange.size();
			createInfo.pCode = (uint32_t*)binaryShader.VertexRange.data();

			VK_CHECK(vkCreateShaderModule(m_Device.Device, &createInfo, nullptr, &vertexModule));
			pushRange.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (desc.Type == EShaderType::EPixel || desc.Type == EShaderType::EGraphics) {

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.codeSize = binaryShader.PixelRange.size();
			createInfo.pCode = (uint32_t*)binaryShader.PixelRange.data();

			VK_CHECK(vkCreateShaderModule(m_Device.Device, &createInfo, nullptr, &pixelModule));
			pushRange.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (desc.Type == EShaderType::ECompute) {

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.codeSize = binaryShader.ComputeRange.size();
			createInfo.pCode = (uint32_t*)binaryShader.ComputeRange.data();

			VK_CHECK(vkCreateShaderModule(m_Device.Device, &createInfo, nullptr, &computeModule));
			pushRange.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
		}

		std::vector<VkDescriptorSetLayout> vkLayouts;
		{
			for (auto layout : layouts) {

				VulkanRHIBindingSetLayout* vkLayout = (VulkanRHIBindingSetLayout*)layout->GetRHIData();
				vkLayouts.push_back(vkLayout->Layout);
			}
		}

		VkPipelineLayoutCreateInfo layoutInfo = VulkanUtils::PipelineLayoutCreateInfo();
		layoutInfo.setLayoutCount = (uint32_t)vkLayouts.size();
		layoutInfo.pSetLayouts = vkLayouts.data();
		layoutInfo.pPushConstantRanges = (binaryShader.ShaderData.PushDataSize > 0) ? &pushRange : nullptr;
		layoutInfo.pushConstantRangeCount = (binaryShader.ShaderData.PushDataSize > 0) ? 1 : 0;

		VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layoutInfo, nullptr, &shader->PipelineLayout));

		if (desc.Type == EShaderType::ECompute) {

			VkPipelineShaderStageCreateInfo stageInfo = VulkanUtils::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeModule, "CSMain");

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.stage = stageInfo;
			pipelineInfo.layout = shader->PipelineLayout;

			VK_CHECK(vkCreateComputePipelines(m_Device.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader->Pipeline));
			vkDestroyShaderModule(m_Device.Device, computeModule, nullptr);
		}
		else {

			VkCullModeFlagBits cullMode = desc.CullBackFaces ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;

			std::vector<VkFormat> colorAttachmentFormats;
			colorAttachmentFormats.reserve(desc.ColorTargetFormats.size());
			for (auto f : desc.ColorTargetFormats) {

				colorAttachmentFormats.push_back(VulkanUtils::TextureFormatToVulkan(f));
			}

			GraphicsPipelineBuilder builder;
			builder.SetShaders(vertexModule, pixelModule);
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			builder.SetCullMode(cullMode, VulkanUtils::FrontFaceToVulkan(desc.FrontFace));
			builder.SetMultisamplingNone();
			builder.SetColorAttachments(colorAttachmentFormats.data(), (uint32_t)colorAttachmentFormats.size());

			if (desc.EnableAlphaBlend) {
				builder.EnableBlendingAlphablend();
			}
			else if (desc.EnableAdditiveBlend) {
				builder.EnableBlendingAdditive();
			}
			else {
				builder.DisableBlending();
			}

			if (desc.EnableDepthTest) {

				builder.SetDepthFormat(VK_FORMAT_D32_SFLOAT);
				builder.EnableDepthTest(desc.EnableDepthWrite, VK_COMPARE_OP_GREATER_OR_EQUAL);
			}
			else {
				builder.DisableDepthTest();
			}

			builder.PipelineLayout = shader->PipelineLayout;
			shader->Pipeline = builder.BuildPipeline(m_Device.Device);

			vkDestroyShaderModule(m_Device.Device, vertexModule, nullptr);
			vkDestroyShaderModule(m_Device.Device, pixelModule, nullptr);
		}

		return (RHIData)shader;
	}

	void VulkanRHIDevice::DestroyShaderRHI(RHIData data) {

		VulkanRHIShader* vkShader = (VulkanRHIShader*)data;

		if (vkShader->PipelineLayout) {
			vkDestroyPipelineLayout(m_Device.Device, vkShader->PipelineLayout, nullptr);
		}

		if (vkShader->Pipeline) {
			vkDestroyPipeline(m_Device.Device, vkShader->Pipeline, nullptr);
		}

		delete vkShader;
	}

	void VulkanRHIDevice::BindShader(RHICommandBuffer* cmd, RHIShader* shader, std::vector<RHIBindingSet*> shaderSets, void* pushData) {

		VulkanRHIShader* vkShader = (VulkanRHIShader*)shader->GetRHIData();
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		VkPipelineBindPoint bindPoint = shader->GetShaderType() == EShaderType::ECompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkCmdBindPipeline(vkCmd->Cmd, bindPoint, vkShader->Pipeline);

		for (int i = 0; i < shaderSets.size(); i++) {

			RHIBindingSet* set = shaderSets[i];
			VulkanRHIBindingSet* vkSet = (VulkanRHIBindingSet*)set->GetRHIData();

			if (set->GetWrites().size() > 0) {

				std::vector<VkWriteDescriptorSet> writes;
				std::deque<VkDescriptorImageInfo> imageInfos;
				std::deque<VkDescriptorBufferInfo> bufferInfos;

				writes.reserve(set->GetWrites().size());

				for (const auto& w : set->GetWrites()) {

					VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					write.dstBinding = w.Slot;
					write.dstArrayElement = w.ArrayElement;
					write.dstSet = vkSet->Set;
					write.descriptorCount = 1;
					write.descriptorType = VulkanUtils::BindingTypeToVulkan(w.Type);

					//ENGINE_WARN("{0}, {1}", w.Slot, w.ArrayElement);

					if (w.Texture) {

						VulkanRHITextureView* vkView = (VulkanRHITextureView*)w.Texture->GetRHIData();

						VkDescriptorImageInfo& info = imageInfos.emplace_back(
							VkDescriptorImageInfo{
							.sampler = nullptr,
							.imageView = vkView->View,
							.imageLayout = VulkanUtils::GPUAccessToVulkanLayout(w.TextureAccess)
							});
						write.pImageInfo = &info;
					}
					else if (w.Buffer) {

						VulkanRHIBuffer* vkBuff = (VulkanRHIBuffer*)w.Buffer->GetRHIData();

						VkDescriptorBufferInfo& info = bufferInfos.emplace_back(
							VkDescriptorBufferInfo{
							.buffer = vkBuff->Buffer,
							.offset = w.BufferOffset,
							.range = w.BufferRange
							});
						write.pBufferInfo = &info;
					}
					else if (w.Sampler) {

						VulkanRHISampler* vkSampler = (VulkanRHISampler*)w.Sampler->GetRHIData();

						VkDescriptorImageInfo& info = imageInfos.emplace_back(
							VkDescriptorImageInfo{
							.sampler = vkSampler->Sampler,
							.imageView = nullptr,
							.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
							});
						write.pImageInfo = &info;
					}

					writes.push_back(write);
				}

				vkUpdateDescriptorSets(m_Device.Device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

				set->ClearWrites();
				writes.clear();
				imageInfos.clear();
				bufferInfos.clear();
			}

			vkCmdBindDescriptorSets(vkCmd->Cmd, bindPoint, vkShader->PipelineLayout, i, 1, &vkSet->Set, 0, nullptr);
		}

		if (pushData) {

			VkShaderStageFlags shaderStage = 0;
			if (shader->GetShaderType() == EShaderType::EVertex || shader->GetShaderType() == EShaderType::EGraphics) shaderStage |= VK_SHADER_STAGE_VERTEX_BIT;
			if (shader->GetShaderType() == EShaderType::EPixel || shader->GetShaderType() == EShaderType::EGraphics) shaderStage |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (shader->GetShaderType() == EShaderType::ECompute) shaderStage |= VK_SHADER_STAGE_COMPUTE_BIT;

			vkCmdPushConstants(vkCmd->Cmd, vkShader->PipelineLayout, shaderStage, 0, shader->GetPushDataSize(), pushData);
		}
	}

	RHIData VulkanRHIDevice::CreateSamplerRHI(const SamplerDesc& desc) {

		VulkanRHISampler* sampler = new VulkanRHISampler();

		VkSamplerCreateInfo createInfo = {};
		createInfo.pNext = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VulkanUtils::SamplerFilterToVulkan(desc.Filter);
		createInfo.minFilter = VulkanUtils::SamplerFilterToVulkan(desc.Filter);
		createInfo.mipmapMode = VulkanUtils::SamplerMipMapModeToVulkan(desc.Filter);
		createInfo.addressModeU = VulkanUtils::SamplerAddressToVulkan(desc.AddressU);
		createInfo.addressModeV = VulkanUtils::SamplerAddressToVulkan(desc.AddressV);
		createInfo.addressModeW = VulkanUtils::SamplerAddressToVulkan(desc.AddressW);
		createInfo.minLod = desc.MinLOD;
		createInfo.maxLod = desc.MaxLOD;
		createInfo.anisotropyEnable = desc.MaxAnisotropy > 0;
		createInfo.maxAnisotropy = desc.MaxAnisotropy;

		VkSamplerReductionModeCreateInfoEXT createInfoReduction{};
		if (desc.Reduction != ESamplerReduction::ENone) {

			createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;
			createInfoReduction.reductionMode = VulkanUtils::SamplerReductionToVulkan(desc.Reduction);
			createInfo.pNext = &createInfoReduction;
		}

		VK_CHECK(vkCreateSampler(m_Device.Device, &createInfo, 0, &sampler->Sampler));
		return (RHIData)sampler;
	}

	void VulkanRHIDevice::DestroySamplerRHI(RHIData data) {

		VulkanRHISampler* vkSampler = (VulkanRHISampler*)data;

		if (vkSampler->Sampler) {
			vkDestroySampler(m_Device.Device, vkSampler->Sampler, nullptr);
		}

		delete vkSampler;
	}

	RHIData VulkanRHIDevice::CreateCommandBufferRHI() {

		VulkanRHICommandBuffer* cmd = new VulkanRHICommandBuffer();

		VkCommandPoolCreateInfo commandPoolInfo = VulkanUtils::CommandPoolCreateInfo(m_Device.Queues.GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VK_CHECK(vkCreateCommandPool(m_Device.Device, &commandPoolInfo, nullptr, &cmd->Pool));

		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanUtils::CommandBufferAllocInfo(cmd->Pool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_Device.Device, &cmdAllocInfo, &cmd->Cmd));

		return (RHIData)cmd;
	}

	void VulkanRHIDevice::DestroyCommandBufferRHI(RHIData data) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)data;

		if (vkCmd->Pool) {
			vkDestroyCommandPool(m_Device.Device, vkCmd->Pool, nullptr);
		}

		delete vkCmd;
	}

	void VulkanRHIDevice::BeginFrameCommandBuffer(RHICommandBuffer* cmd) {
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanUtils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(vkCmd->Cmd, &cmdBeginInfo));

		m_GuiTextureManager.DynamicTexSetsIndex = 0;
	}

	void VulkanRHIDevice::WaitForFrameCommandBuffer(RHICommandBuffer* cmd) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		uint32_t frameIndex = GFrameRenderer->GetFrameCount() % 2;
		VK_CHECK(vkWaitForFences(m_Device.Device, 1, &m_SyncObjects[frameIndex].RenderFence, true, 1000000000));
		VK_CHECK(vkResetFences(m_Device.Device, 1, &m_SyncObjects[frameIndex].RenderFence));
		VK_CHECK(vkResetCommandBuffer(vkCmd->Cmd, 0));
	}

	void VulkanRHIDevice::ImmediateSubmit(std::function<void(RHICommandBuffer*)>&& func) {

		if (!m_ImmCmd) {
			m_ImmCmd = new RHICommandBuffer();
			m_ImmCmd->InitRHI();
		}

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)m_ImmCmd->GetRHIData();

		VK_CHECK(vkResetFences(m_Device.Device, 1, &m_ImmFence));
		VK_CHECK(vkResetCommandBuffer(vkCmd->Cmd, 0));

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanUtils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(vkCmd->Cmd, &cmdBeginInfo));

		func(m_ImmCmd);

		VK_CHECK(vkEndCommandBuffer(vkCmd->Cmd));

		VkCommandBufferSubmitInfo cmdInfo = VulkanUtils::CommandBufferSubmitInfo(vkCmd->Cmd);
		VkSubmitInfo2 submit = VulkanUtils::SubmitInfo(&cmdInfo, nullptr, nullptr);

		VK_CHECK(vkQueueSubmit2(m_Device.Queues.GraphicsQueue, 1, &submit, m_ImmFence));
		VK_CHECK(vkWaitForFences(m_Device.Device, 1, &m_ImmFence, true, 9999999999));
	}

	void VulkanRHIDevice::DispatchCompute(RHICommandBuffer* cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		vkCmdDispatch(vkCmd->Cmd, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanRHIDevice::WaitGPUIdle() {

		vkDeviceWaitIdle(m_Device.Device);
	}

	void VulkanRHIDevice::BeginRendering(RHICommandBuffer* cmd, const RenderInfo& info) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();

		std::vector<VkRenderingAttachmentInfo> colorAttachments;
		colorAttachments.reserve(info.ColorTargets.size());

		VkClearValue depthClear{};
		if (info.DepthClear) {
			depthClear = { .depthStencil = {info.DepthClear->x, (uint32_t)info.DepthClear->y} };
		}

		VkClearValue colorClear{};
		if (info.ColorClear) {
			colorClear = { .color = {info.ColorClear->x, info.ColorClear->y, info.ColorClear->z, info.ColorClear->w} };
		}

		for (auto v : info.ColorTargets) {

			VulkanRHITextureView* vkView = (VulkanRHITextureView*)v->GetRHIData();
			colorAttachments.push_back(VulkanUtils::AttachmentInfo(vkView->View, info.ColorClear ? &colorClear : nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
		}

		VkRenderingAttachmentInfo depthAttachment{};
		if (info.DepthTarget) {

			VulkanRHITextureView* vkView = (VulkanRHITextureView*)info.DepthTarget->GetRHIData();
			depthAttachment = VulkanUtils::DepthAttachmentInfo(vkView->View, info.DepthClear ? &depthClear : nullptr, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		}

		VkRenderingInfo renderInfo = VulkanUtils::RenderingInfo({info.DrawSize.x, info.DrawSize.y}, colorAttachments.size() > 0 ? colorAttachments.data() : nullptr, (uint32_t)colorAttachments.size(),
			info.DepthTarget ? &depthAttachment : nullptr);
		vkCmdBeginRendering(vkCmd->Cmd, &renderInfo);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)info.DrawSize.x;
		viewport.height = (float)info.DrawSize.y;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;
		vkCmdSetViewport(vkCmd->Cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = info.DrawSize.x;
		scissor.extent.height = info.DrawSize.y;
		vkCmdSetScissor(vkCmd->Cmd, 0, 1, &scissor);
	}

	void VulkanRHIDevice::EndRendering(RHICommandBuffer* cmd) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		vkCmdEndRendering(vkCmd->Cmd);
	}

	void VulkanRHIDevice::DrawIndirectCount(RHICommandBuffer* cmd, RHIBuffer* commBuffer, size_t offset, RHIBuffer* countBuffer,
		size_t countBufferOffset, uint32_t maxDrawCount, uint32_t commStride) 
	{
		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		VulkanRHIBuffer* vkCommBuff = (VulkanRHIBuffer*)commBuffer->GetRHIData();
		VulkanRHIBuffer* vkCountBuff = (VulkanRHIBuffer*)countBuffer->GetRHIData();

		vkCmdDrawIndirectCount(vkCmd->Cmd, vkCommBuff->Buffer, offset, vkCountBuff->Buffer, 
			countBufferOffset, maxDrawCount, commStride);

		//vkCmdDrawIndirect(vkCmd->Cmd, vkCommBuff->Buffer, offset, 1, commStride);
	}

	void VulkanRHIDevice::Draw(RHICommandBuffer* cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		vkCmdDraw(vkCmd->Cmd, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanRHIDevice::UpdateImGuiObjects(ImGuiRTState* state) {
		uint32_t frameIndex = GFrameRenderer->GetFrameCount() % 2;

		if (m_ImGuiDrawObjects[frameIndex].IdxBufferSize < state->TotalIdxCount || !m_ImGuiDrawObjects[frameIndex].IdxBuffer) {
			if (m_ImGuiDrawObjects[frameIndex].IdxBuffer) {
				DestroyBufferRHI((RHIData)m_ImGuiDrawObjects[frameIndex].IdxBuffer);
			}

			BufferDesc desc{};
			desc.Size = state->TotalIdxCount * sizeof(ImDrawIdx);
			desc.UsageFlags = EBufferUsageFlags::EIndex;
			desc.MemUsage = EBufferMemUsage::ECPUToGPU;

			m_ImGuiDrawObjects[frameIndex].IdxBuffer = (VulkanRHIBuffer*)CreateBufferRHI(desc);
			m_ImGuiDrawObjects[frameIndex].IdxBufferSize = state->TotalIdxCount;
		}
		if (m_ImGuiDrawObjects[frameIndex].VtxBufferSize < state->TotalVtxCount || !m_ImGuiDrawObjects[frameIndex].VtxBuffer) {
			if (m_ImGuiDrawObjects[frameIndex].VtxBuffer) {
				DestroyBufferRHI((RHIData)m_ImGuiDrawObjects[frameIndex].VtxBuffer);
			}

			BufferDesc desc{};
			desc.Size = state->TotalVtxCount * sizeof(ImDrawVert);
			desc.UsageFlags = EBufferUsageFlags::EStorage;
			desc.MemUsage = EBufferMemUsage::ECPUToGPU; 

			m_ImGuiDrawObjects[frameIndex].VtxBuffer = (VulkanRHIBuffer*)CreateBufferRHI(desc);
			m_ImGuiDrawObjects[frameIndex].VtxBufferSize = state->TotalVtxCount;
		}

		size_t vtxOffset = 0;
		size_t idxOffset = 0;

		for (const auto& list : state->CmdLists) {
			memcpy((ImDrawVert*)m_ImGuiDrawObjects[frameIndex].VtxBuffer->AllocationInfo.pMappedData + vtxOffset,
				list.VtxBuffer.Data, list.VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy((ImDrawIdx*)m_ImGuiDrawObjects[frameIndex].IdxBuffer->AllocationInfo.pMappedData + idxOffset,
				list.IdxBuffer.Data, list.IdxBuffer.Size * sizeof(ImDrawIdx));

			vtxOffset += list.VtxBuffer.Size;
			idxOffset += list.IdxBuffer.Size;
		}
	}

	void VulkanRHIDevice::DrawSwapchain(RHICommandBuffer* cmd, uint32_t width, uint32_t height, ImGuiRTState* guiState, RHITexture2D* fillTexture) {

		VulkanRHICommandBuffer* vkCmd = (VulkanRHICommandBuffer*)cmd->GetRHIData();
		uint32_t frameIndex = GFrameRenderer->GetFrameCount() % 2;

		uint32_t swapchainImageIndex = 0;
		VkResult check = vkAcquireNextImageKHR(m_Device.Device, m_Swapchain.Swapchain, 1000000000, m_SyncObjects[frameIndex].SwapchainSemaphore, nullptr, &swapchainImageIndex);
		if (check == VK_ERROR_OUT_OF_DATE_KHR) {
			m_Swapchain.Resize(m_Device, width, height);
			return;
		}

		bool drawGui = (guiState != nullptr);
		auto renderImGui = [&, this](bool clearSwapchain) {
			VkRenderingAttachmentInfo colorAttachment{};
			if (clearSwapchain) {
				VkClearValue colorClear = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
				colorAttachment = VulkanUtils::AttachmentInfo(m_Swapchain.ImageViews[swapchainImageIndex], &colorClear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
			else { 
				colorAttachment = VulkanUtils::AttachmentInfo(m_Swapchain.ImageViews[swapchainImageIndex], nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			VkRenderingInfo renderInfo = VulkanUtils::RenderingInfo(m_Swapchain.Extent, &colorAttachment, 1, nullptr);

			vkCmdBeginRendering(vkCmd->Cmd, &renderInfo); 
			// code based on imgui_impl_vulkan.cpp ImGui_ImplVulkan_RenderDrawData function
			{
				int fb_width = (int)(guiState->DisplaySize.x * guiState->FramebufferScale.x);
				int fb_height = (int)(guiState->DisplaySize.y * guiState->FramebufferScale.y);

				if (guiState->TotalVtxCount > 0) {
					UpdateImGuiObjects(guiState);

					if (!m_ImGuiShader) {
						ShaderDesc desc{};
						desc.Type = EShaderType::EGraphics;
						desc.Name = "EditorGUI";
						desc.ColorTargetFormats = { ETextureFormat::EBGRA8U };
						desc.EnableAlphaBlend = true;

						RHIShader* shader = GShaderManager->GetShaderFromCache(desc);
						m_ImGuiShader = (VulkanRHIShader*)shader->GetRHIData();
						m_ImGuiLayout = (VulkanRHIBindingSetLayout*)shader->GetLayouts()[0]->GetRHIData();
						m_ImGuiTexLayout = (VulkanRHIBindingSetLayout*)shader->GetLayouts()[1]->GetRHIData();

						m_GuiTextureManager.Pool = m_GlobalSetPool;
						m_GuiTextureManager.Device = m_Device.Device;
						m_GuiTextureManager.Layout = m_ImGuiTexLayout->Layout;
						m_GuiTextureManager.Init();
					}

					// setup render state
					{
						vkCmdBindPipeline(vkCmd->Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ImGuiShader->Pipeline);
						vkCmdBindIndexBuffer(vkCmd->Cmd, m_ImGuiDrawObjects[frameIndex].IdxBuffer->Buffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

						VkViewport viewport{};
						viewport.x = 0;
						viewport.y = 0; 
						viewport.width = (float)fb_width;
						viewport.height = (float)fb_height;
						viewport.minDepth = 0.0f;
						viewport.maxDepth = 1.0f;
						vkCmdSetViewport(vkCmd->Cmd, 0, 1, &viewport);

						{
							VkDescriptorSet set{};

							VkDescriptorSetAllocateInfo info = {};
							info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
							info.descriptorPool = m_GlobalSetPool;
							info.descriptorSetCount = 1;
							info.pSetLayouts = &m_ImGuiLayout->Layout;
							VK_CHECK(vkAllocateDescriptorSets(m_Device.Device, &info, &set));

							{
								VkDescriptorBufferInfo bufferInfo{};
								bufferInfo.offset = 0;
								bufferInfo.range = guiState->TotalVtxCount * sizeof(ImDrawVert);
								bufferInfo.buffer = m_ImGuiDrawObjects[frameIndex].VtxBuffer->Buffer;

								VkWriteDescriptorSet write{};
								write.dstBinding = 0;
								write.dstArrayElement = 0;
								write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
								write.dstSet = set;
								write.descriptorCount = 1;
								write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
								write.pBufferInfo = &bufferInfo;

								vkUpdateDescriptorSets(m_Device.Device, 1, &write, 0, nullptr);
								vkCmdBindDescriptorSets(vkCmd->Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ImGuiShader->PipelineLayout, 0, 1, &set, 0, nullptr);
							}

							GFrameRenderer->SubmitToFrameQueue([set, this]() {
								vkFreeDescriptorSets(m_Device.Device, m_GlobalSetPool, 1, &set);
								});
						}
					}

					ImVec2 clip_off = guiState->DisplayPos;
					ImVec2 clip_scale = guiState->FramebufferScale;

					int global_vtx_offset = 0;
					int global_idx_offset = 0;
					for (auto& list : guiState->CmdLists) {
						for (int cmd_i = 0; cmd_i < list.CmdBuffer.size(); cmd_i++) {
							const ImDrawCmd& cmd = list.CmdBuffer[cmd_i];

							ImVec2 clip_min((cmd.ClipRect.x - clip_off.x) * clip_scale.x, (cmd.ClipRect.y - clip_off.y) * clip_scale.y);
							ImVec2 clip_max((cmd.ClipRect.z - clip_off.x) * clip_scale.x, (cmd.ClipRect.w - clip_off.y) * clip_scale.y);

							if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
							if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
							if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
							if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
							if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
								continue;

							// Apply scissor/clipping rectangle
							VkRect2D scissor;
							scissor.offset.x = (int32_t)(clip_min.x);
							scissor.offset.y = (int32_t)(clip_min.y);
							scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
							scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
							vkCmdSetScissor(vkCmd->Cmd, 0, 1, &scissor);

							VkDescriptorSet texSet = nullptr;
							RHITexture2D* tex = (RHITexture2D*)cmd.GetTexID();

							if (tex) {
								VulkanRHITextureView* vkView = (VulkanRHITextureView*)tex->GetTextureView()->GetRHIData();
								VulkanRHISampler* vkSampler = (VulkanRHISampler*)tex->GetSampler()->GetRHIData();

								texSet = m_GuiTextureManager.GetTextureSet(vkView->View, vkSampler->Sampler, frameIndex);
							}
							vkCmdBindDescriptorSets(vkCmd->Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ImGuiShader->PipelineLayout, 1, 1, &texSet, 0, nullptr);

							struct {
								float Scale[2];
								float Translate[2];
								uint32_t BaseVtx;
								float Padding0[3];
							} pc{};

							pc.Scale[0] = 2.0f / guiState->DisplaySize.x;
							pc.Scale[1] = 2.0f / guiState->DisplaySize.y;
							pc.Translate[0] = -1.0f - guiState->DisplayPos.x * pc.Scale[0];
							pc.Translate[1] = -1.0f - guiState->DisplayPos.y * pc.Scale[1];
							pc.BaseVtx = cmd.VtxOffset + global_vtx_offset;

							vkCmdPushConstants(vkCmd->Cmd, m_ImGuiShader->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

							// Draw
							vkCmdDrawIndexed(vkCmd->Cmd, cmd.ElemCount, 1, cmd.IdxOffset + global_idx_offset, 0, 0);
						}

						global_idx_offset += list.IdxBuffer.Size;
						global_vtx_offset += list.VtxBuffer.Size;
					}
				}
			}
			vkCmdEndRendering(vkCmd->Cmd);
			};

		if (fillTexture) {
			VulkanRHITexture* vkFillTex = (VulkanRHITexture*)fillTexture->GetRHIData();

			VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			VulkanUtils::BarrierImage(vkCmd->Cmd, vkFillTex->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			// copy fill texture to swapchain
			{
				VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

				blitRegion.srcOffsets[1].x = fillTexture->GetSizeXYZ().x;
				blitRegion.srcOffsets[1].y = fillTexture->GetSizeXYZ().y;
				blitRegion.srcOffsets[1].z = 1;

				blitRegion.dstOffsets[1].x = m_Swapchain.Extent.width;
				blitRegion.dstOffsets[1].y = m_Swapchain.Extent.height;
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
				blitInfo.dstImage = m_Swapchain.Images[swapchainImageIndex];
				blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				blitInfo.srcImage = vkFillTex->Image;
				blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				blitInfo.filter = VK_FILTER_LINEAR;
				blitInfo.regionCount = 1;
				blitInfo.pRegions = &blitRegion;

				vkCmdBlitImage2(vkCmd->Cmd, &blitInfo);
			}

			if (drawGui) {

				VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				renderImGui(false);
				VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}
			else {
				VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}
		}
		else if (drawGui) {

			VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			renderImGui(true);
			VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}
		else {

			ENGINE_WARN("Not rendering anything to swapchain at frame: {0}! This should not happen.", GFrameRenderer->GetFrameCount());
			VulkanUtils::BarrierImage(vkCmd->Cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}

		// submit all commands from current frame
		{
			VK_CHECK(vkEndCommandBuffer(vkCmd->Cmd));

			VkCommandBufferSubmitInfo cmdInfo = VulkanUtils::CommandBufferSubmitInfo(vkCmd->Cmd);

			VkSemaphoreSubmitInfo waitInfo = VulkanUtils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, m_SyncObjects[frameIndex].SwapchainSemaphore);
			VkSemaphoreSubmitInfo signalInfo = VulkanUtils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_SyncObjects[frameIndex].RenderSemaphore);

			VkSubmitInfo2 submit = VulkanUtils::SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

			VK_CHECK(vkQueueSubmit2(m_Device.Queues.GraphicsQueue, 1, &submit, m_SyncObjects[frameIndex].RenderFence));

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.pSwapchains = &m_Swapchain.Swapchain;
			presentInfo.swapchainCount = 1;
			presentInfo.pWaitSemaphores = &m_SyncObjects[frameIndex].RenderSemaphore;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pImageIndices = &swapchainImageIndex;

			VkResult check2 = vkQueuePresentKHR(m_Device.Queues.GraphicsQueue, &presentInfo);
			if (check2 == VK_ERROR_OUT_OF_DATE_KHR) {
				m_Swapchain.Resize(m_Device, width, height);
					};
		}
	}


	void VulkanImGuiTextureManager::Init() {
		for (int f = 0; f < 2; f++) {
			for (uint32_t t = 0; t < 50; t++) {
				VkDescriptorSet* set = &Sets[t][f];

				VkDescriptorSetAllocateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				info.descriptorPool = Pool;
				info.descriptorSetCount = 1;
				info.pSetLayouts = &Layout;
				VK_CHECK(vkAllocateDescriptorSets(Device, &info, set));
			}
		}
	}

	void VulkanImGuiTextureManager::Cleanup() {
		for (int f = 0; f < 2; f++) {
			for (int i = 0; i < 50; i++) {
				vkFreeDescriptorSets(Device, Pool, 1, &Sets[i][f]);
			}
		}
	}

	VkDescriptorSet VulkanImGuiTextureManager::GetTextureSet(VkImageView view, VkSampler sampler, uint32_t currentFrameIndex) {
		VkDescriptorSet set = Sets[DynamicTexSetsIndex][currentFrameIndex];
		DynamicTexSetsIndex++;

		{
			VkDescriptorImageInfo imageDesc{};
			imageDesc.sampler = nullptr;
			imageDesc.imageView = view;
			imageDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo samplerDesc{};
			samplerDesc.sampler = sampler;
			samplerDesc.imageView = nullptr;
			samplerDesc.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VkWriteDescriptorSet writes[2];
			writes[0] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			writes[0].dstBinding = 0;
			writes[0].dstArrayElement = 0;
			writes[0].dstSet = set;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[0].pImageInfo = &imageDesc;

			writes[1] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			writes[1].dstBinding = 1;
			writes[1].dstArrayElement = 0;
			writes[1].dstSet = set;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			writes[1].pImageInfo = &samplerDesc;

			vkUpdateDescriptorSets(Device, 2, writes, 0, nullptr);
		}
		return set;
	}
}