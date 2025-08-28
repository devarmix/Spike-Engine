#include <Backends/Vulkan/VulkanGfxDevice.h>
#include <Backends/Vulkan/VulkanResources.h>
#include <Backends/Vulkan/VulkanUtils.h>
#include <Backends/Vulkan/VulkanPipeline.h>
#include <Engine/Renderer/FrameRenderer.h>

#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

namespace Spike {

	VulkanRHIDevice::VulkanRHIDevice(Window* window, bool useImgui) {

		m_Device.Init(window, true);
		m_Swapchain.Init(m_Device, window->GetWidth(), window->GetHeight());

		m_UsingImGui = useImgui;

		if (useImgui) {

			VkDescriptorPoolSize pool_Sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

			VkDescriptorPoolCreateInfo pool_Info = {};
			pool_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_Info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_Info.maxSets = 1000;
			pool_Info.poolSizeCount = (uint32_t)std::size(pool_Sizes);
			pool_Info.pPoolSizes = pool_Sizes;

			VK_CHECK(vkCreateDescriptorPool(m_Device.Device, &pool_Info, nullptr, &m_ImGuiPool));

			ImGui::CreateContext();

			ImGui_ImplSDL2_InitForVulkan((SDL_Window*)window->GetNativeWindow());

			ImGui_ImplVulkan_InitInfo init_Info = {};
			init_Info.Instance = m_Device.Instance;
			init_Info.PhysicalDevice = m_Device.PhysicalDevice;
			init_Info.Device = m_Device.Device;
			init_Info.Queue = m_Device.Queues.GraphicsQueue;
			init_Info.DescriptorPool = m_ImGuiPool;
			init_Info.MinImageCount = 3;
			init_Info.ImageCount = 3;
			init_Info.UseDynamicRendering = true;

			init_Info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
			init_Info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
			init_Info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_Swapchain.Format;

			init_Info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

			ImGui_ImplVulkan_Init(&init_Info);

			ImGui_ImplVulkan_CreateFontsTexture();

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

			m_GuiTextureManager.Init(50);
		}
		else {
			m_ImGuiPool = nullptr;
		}

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
	}

	VulkanRHIDevice::~VulkanRHIDevice() {

		for (int i = 0; i < 2; i++) {

			vkDestroyFence(m_Device.Device, m_SyncObjects[i].RenderFence, nullptr);
			vkDestroySemaphore(m_Device.Device, m_SyncObjects[i].RenderSemaphore, nullptr);
			vkDestroySemaphore(m_Device.Device, m_SyncObjects[i].SwapchainSemaphore, nullptr);
		}

		if (m_UsingImGui) {

			m_GuiTextureManager.Cleanup();
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(m_Device.Device, m_ImGuiPool, nullptr);
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

	RHIData* VulkanRHIDevice::CreateTexture2DRHI(const Texture2DDesc& desc) {

		VulkanRHITexture* tex = new VulkanRHITexture();

		VkFormat vkFormat = VulkanUtils::TextureFormatToVulkan(desc.Format);
		VkImageUsageFlags vkFlags = VulkanUtils::TextureUsageFlagsToVulkan(desc.UsageFlags);

		VkImageCreateInfo imgInfo = VulkanUtils::ImageCreateInfo(vkFormat, vkFlags, { desc.Width, desc.Height, 1 });
		imgInfo.mipLevels = desc.NumMips;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(m_Device.Allocator, &imgInfo, &allocInfo, &tex->NativeData.Image, &tex->NativeData.Allocation, nullptr));

		if (desc.PixelData) {

			size_t dataSize = 0;
			if (vkFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
				dataSize = (size_t)desc.Width * desc.Height * 16;
			}
			else {
				dataSize = (size_t)desc.Width * desc.Height * 4;
			}

			BufferDesc uploadDesc{};
			uploadDesc.Size = dataSize;
			uploadDesc.UsageFlags = EBufferUsageFlags::ECopySrc;
			uploadDesc.MemUsage = EBufferMemUsage::ECPUToGPU;

			VulkanRHIBuffer* uploadBuffer = (VulkanRHIBuffer*)CreateBufferRHI(uploadDesc);

			memcpy(uploadBuffer->NativeData.AllocationInfo.pMappedData, desc.PixelData, dataSize);
			ImmediateSubmit([&](RHICommandBuffer* cmd) {

				VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
				VulkanUtils::BarrierImage(vkCmd->Cmd, tex->NativeData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = { desc.Width, desc.Height, 1 };

				// copy the buffer into the image
				vkCmdCopyBufferToImage(vkCmd->Cmd, uploadBuffer->NativeData.Buffer, tex->NativeData.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
					&copyRegion);

				if (desc.NumMips > 1) {
					VulkanUtils::GenerateMipmaps(vkCmd->Cmd, tex->NativeData.Image, VkExtent2D{ desc.Width, desc.Height });
				}
				else {
					VulkanUtils::BarrierImage(vkCmd->Cmd, tex->NativeData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}
				});

			//uploadbuffer.Destroy(&m_Device);
			DestroyBufferRHI(uploadBuffer);
		}

		return tex;
	}

	void VulkanRHIDevice::BarrierTexture2D(RHICommandBuffer* cmd, RHITexture2D* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		VulkanRHITexture::Data* vkTex = (VulkanRHITexture::Data*)texture->GetRHIData()->GetNativeData();

		VkImageLayout vkLastLayout = VulkanUtils::GPUAccessToVulkanLayout(lastAccess);
		VkAccessFlags2 vkLastAccess = VulkanUtils::GPUAccessToVulkanAccess(lastAccess);
		VkPipelineStageFlags2 vkLastStage = VulkanUtils::GPUAccessToVulkanStage(lastAccess);

		VkImageLayout vkNewLayout = VulkanUtils::GPUAccessToVulkanLayout(newAccess);
		VkAccessFlags2 vkNewAccess = VulkanUtils::GPUAccessToVulkanAccess(newAccess);
		VkPipelineStageFlags2 vkNewStage = VulkanUtils::GPUAccessToVulkanStage(newAccess);

		VkImageAspectFlags aspect = texture->GetFormat() == ETextureFormat::ED32F ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		VulkanUtils::BarrierImage(vkCmd->Cmd, vkTex->Image, aspect, vkLastLayout, vkNewLayout, vkLastAccess, vkNewAccess, vkLastStage, vkNewStage);
	}

	void VulkanRHIDevice::DestroyTexture2DRHI(RHIData* data) {

		VulkanRHITexture::Data* vkTex = (VulkanRHITexture::Data*)data->GetNativeData();

		if (vkTex->Image) {
			vmaDestroyImage(m_Device.Allocator, vkTex->Image, vkTex->Allocation);
		}

		delete data; 
	}

	RHIData* VulkanRHIDevice::CreateCubeTextureRHI(const CubeTextureDesc& desc) {

		VulkanRHITexture* tex = new VulkanRHITexture();

		VkFormat vkFormat = VulkanUtils::TextureFormatToVulkan(desc.Format);
		VkImageUsageFlags vkFlags = VulkanUtils::TextureUsageFlagsToVulkan(desc.UsageFlags);

		VkImageCreateInfo imgInfo = VulkanUtils::CubeImageCreateInfo(vkFormat, vkFlags, desc.Size);
		imgInfo.mipLevels = desc.NumMips;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(m_Device.Allocator, &imgInfo, &allocInfo, &tex->NativeData.Image, &tex->NativeData.Allocation, nullptr));

		return tex; 
	}

	void VulkanRHIDevice::BarrierCubeTexture(RHICommandBuffer* cmd, RHICubeTexture* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		VulkanRHITexture::Data* vkTex = (VulkanRHITexture::Data*)texture->GetRHIData()->GetNativeData();

		VkImageLayout vkLastLayout = VulkanUtils::GPUAccessToVulkanLayout(lastAccess);
		VkAccessFlags2 vkLastAccess = VulkanUtils::GPUAccessToVulkanAccess(lastAccess);
		VkPipelineStageFlags2 vkLastStage = VulkanUtils::GPUAccessToVulkanStage(lastAccess);

		VkImageLayout vkNewLayout = VulkanUtils::GPUAccessToVulkanLayout(newAccess);
		VkAccessFlags2 vkNewAccess = VulkanUtils::GPUAccessToVulkanAccess(newAccess);
		VkPipelineStageFlags2 vkNewStage = VulkanUtils::GPUAccessToVulkanStage(newAccess);

		VulkanUtils::BarrierImage(vkCmd->Cmd, vkTex->Image, VK_IMAGE_ASPECT_COLOR_BIT, vkLastLayout, vkNewLayout, vkLastAccess, vkNewAccess, vkLastStage, vkNewStage);
	}

	void VulkanRHIDevice::DestroyCubeTextureRHI(RHIData* data) {

		VulkanRHITexture::Data* vkTex = (VulkanRHITexture::Data*)data->GetNativeData();

		if (vkTex->Image) {
			vmaDestroyImage(m_Device.Allocator, vkTex->Image, vkTex->Allocation);
		}

		delete data;
	}

	void VulkanRHIDevice::CopyTexture(RHICommandBuffer* cmd, RHITexture* src, const TextureCopyRegion& srcRegion, RHITexture* dst, 
		const TextureCopyRegion& dstRegion, Vec2Uint copySize) 
	{
		VulkanRHITexture::Data* vkSrc = (VulkanRHITexture::Data*)src->GetRHIData()->GetNativeData();
		VulkanRHITexture::Data* vkDst = (VulkanRHITexture::Data*)dst->GetRHIData()->GetNativeData();
		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

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

	RHIData* VulkanRHIDevice::CreateTextureViewRHI(const TextureViewDesc& desc) {

		VulkanRHITextureView* view = new VulkanRHITextureView();

		VkFormat vkFormat = VulkanUtils::TextureFormatToVulkan(desc.SourceTexture->GetFormat());
		VulkanRHITexture::Data* vkTex = (VulkanRHITexture::Data*)desc.SourceTexture->GetRHIData()->GetNativeData();
		VkImageAspectFlags aspect = vkFormat == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageViewCreateInfo viewInfo = VulkanUtils::ImageViewCreateInfo(vkFormat, vkTex->Image, aspect);
		if (desc.SourceTexture->GetTextureType() == ETextureType::ECube) viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

		viewInfo.subresourceRange.levelCount = desc.NumMips;
		viewInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
		viewInfo.subresourceRange.baseMipLevel = desc.BaseMip;
		viewInfo.subresourceRange.layerCount = desc.NumArrayLayers;

		VK_CHECK(vkCreateImageView(m_Device.Device, &viewInfo, nullptr, &view->NativeData.View));
		
		return view;
	}

	void VulkanRHIDevice::DestroyTextureViewRHI(RHIData* data) {

		VulkanRHITextureView::Data* vkView = (VulkanRHITextureView::Data*)data->GetNativeData();

		if (vkView->View) {
			vkDestroyImageView(m_Device.Device, vkView->View, nullptr);
		}

		delete data;
	}

	RHIData* VulkanRHIDevice::CreateBufferRHI(const BufferDesc& desc) {

		VulkanRHIBuffer* buff = new VulkanRHIBuffer();

		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.flags = 0;
		bufferInfo.size = desc.Size;
		bufferInfo.usage = VulkanUtils::BufferUsageToVulkan(desc.UsageFlags);

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = VulkanUtils::BufferMemUsageToVulkan(desc.MemUsage);
		vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_CHECK(vmaCreateBuffer(m_Device.Allocator, &bufferInfo, &vmaAllocInfo, &buff->NativeData.Buffer,
			&buff->NativeData.Allocation, &buff->NativeData.AllocationInfo));

		return buff;
	}

	void VulkanRHIDevice::DestroyBufferRHI(RHIData* data) {

		VulkanRHIBuffer::Data* vkBuff = (VulkanRHIBuffer::Data*)data->GetNativeData();

		if (vkBuff->Buffer) {
			vmaDestroyBuffer(m_Device.Allocator, vkBuff->Buffer, vkBuff->Allocation);
		}

		delete data;
	}

	void VulkanRHIDevice::CopyBuffer(RHICommandBuffer* cmd, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) {

		VulkanRHIBuffer::Data* vkSrc = (VulkanRHIBuffer::Data*)srcBuffer->GetRHIData()->GetNativeData();
		VulkanRHIBuffer::Data* vkDst = (VulkanRHIBuffer::Data*)dstBuffer->GetRHIData()->GetNativeData();
		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

		VkBufferCopy bufferCopy{};
		bufferCopy.dstOffset = dstOffset;
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.size = size;

		vkCmdCopyBuffer(vkCmd->Cmd, vkSrc->Buffer, vkDst->Buffer, 1, &bufferCopy);
	}

	void* VulkanRHIDevice::MapBufferMem(RHIBuffer* buffer) {

		VulkanRHIBuffer::Data* vkBuff = (VulkanRHIBuffer::Data*)buffer->GetRHIData()->GetNativeData();
		return vkBuff->AllocationInfo.pMappedData;
	}

	void VulkanRHIDevice::BarrierBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHIBuffer::Data* vkBuff = (VulkanRHIBuffer::Data*)buffer->GetRHIData()->GetNativeData();
		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

		VkAccessFlags2 vkLastAccess = VulkanUtils::GPUAccessToVulkanAccess(lastAccess);
		VkPipelineStageFlags2 vkLastStage = VulkanUtils::GPUAccessToVulkanStage(lastAccess);

		VkAccessFlags2 vkNewAccess = VulkanUtils::GPUAccessToVulkanAccess(newAccess);
		VkPipelineStageFlags2 vkNewStage = VulkanUtils::GPUAccessToVulkanStage(newAccess);

		VulkanUtils::BarrierBuffer(vkCmd->Cmd, vkBuff->Buffer, size, offset, vkLastAccess, vkNewAccess, vkLastStage, vkNewStage);
	}

	void VulkanRHIDevice::FillBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, uint32_t value, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) {

		VulkanRHIBuffer::Data* vkBuff = (VulkanRHIBuffer::Data*)buffer->GetRHIData()->GetNativeData();
		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

		BarrierBuffer(cmd, buffer, size, offset, lastAccess, EGPUAccessFlags::ECopyDst);
		vkCmdFillBuffer(vkCmd->Cmd, vkBuff->Buffer, offset, size, value);
		BarrierBuffer(cmd, buffer, size, offset, EGPUAccessFlags::ECopyDst, newAccess);
	}

	uint64_t VulkanRHIDevice::GetBufferGPUAddress(RHIBuffer* buffer) {

		VulkanRHIBuffer::Data* vkBuffer = (VulkanRHIBuffer::Data*)buffer->GetRHIData()->GetNativeData();

		VkBufferDeviceAddressInfo vDeviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = vkBuffer->Buffer };
		return vkGetBufferDeviceAddress(m_Device.Device, &vDeviceAddressInfo);
	}

	RHIData* VulkanRHIDevice::CreateBindingSetLayoutRHI(const BindingSetLayoutDesc& desc) {

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

			VK_CHECK(vkCreateDescriptorSetLayout(m_Device.Device, &info, nullptr, &layout->NativeData.Layout));
		}
		else {

			info.pNext = nullptr;
			info.flags = 0;

			VK_CHECK(vkCreateDescriptorSetLayout(m_Device.Device, &info, nullptr, &layout->NativeData.Layout));
		}

		return layout;
	}

	void VulkanRHIDevice::DestroyBindingSetLayoutRHI(RHIData* data) {

		VulkanRHIBindingSetLayout::Data* vkLayout = (VulkanRHIBindingSetLayout::Data*)data->GetNativeData();

		if (vkLayout->Layout) {
			vkDestroyDescriptorSetLayout(m_Device.Device, vkLayout->Layout, nullptr);
		}

		delete data;
	}

	RHIData* VulkanRHIDevice::CreateBindingSetRHI(RHIBindingSetLayout* layout) {

		VulkanRHIBindingSet* set = new VulkanRHIBindingSet();
		VulkanRHIBindingSetLayout::Data* vkLayout = (VulkanRHIBindingSetLayout::Data*)layout->GetRHIData()->GetNativeData();

		set->NativeData.AllocatedPool = layout->GetDesc().UseDescriptorIndexing ? m_BindlessPool : m_GlobalSetPool;

		VkDescriptorSetAllocateInfo SetInfo{};
		SetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetInfo.pNext = nullptr;
		SetInfo.descriptorPool = set->NativeData.AllocatedPool;
		SetInfo.pSetLayouts = &vkLayout->Layout;
		SetInfo.descriptorSetCount = 1;

		VK_CHECK(vkAllocateDescriptorSets(m_Device.Device, &SetInfo, &set->NativeData.Set));

		return set;
	}

	void VulkanRHIDevice::DestroyBindingSetRHI(RHIData* data) {

		VulkanRHIBindingSet::Data* vkSet = (VulkanRHIBindingSet::Data*)data->GetNativeData();

		if (vkSet->Set) {
			vkFreeDescriptorSets(m_Device.Device, vkSet->AllocatedPool, 1, &vkSet->Set);
		}

		delete data;
	}

	RHIData* VulkanRHIDevice::CreateShaderRHI(const ShaderDesc& desc, const ShaderCompiler::BinaryShader& binaryShader, const std::vector<RHIBindingSetLayout*>& layouts) {

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

				VulkanRHIBindingSetLayout::Data* vkLayout = (VulkanRHIBindingSetLayout::Data*)layout->GetRHIData()->GetNativeData();
				vkLayouts.push_back(vkLayout->Layout);
			}
		}

		VkPipelineLayoutCreateInfo layoutInfo = VulkanUtils::PipelineLayoutCreateInfo();
		layoutInfo.setLayoutCount = (uint32_t)vkLayouts.size();
		layoutInfo.pSetLayouts = vkLayouts.data();
		layoutInfo.pPushConstantRanges = (binaryShader.ShaderData.PushDataSize > 0) ? &pushRange : nullptr;
		layoutInfo.pushConstantRangeCount = (binaryShader.ShaderData.PushDataSize > 0) ? 1 : 0;

		VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layoutInfo, nullptr, &shader->NativeData.PipelineLayout));

		if (desc.Type == EShaderType::ECompute) {

			VkPipelineShaderStageCreateInfo stageInfo = VulkanUtils::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeModule, "CSMain");

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.stage = stageInfo;
			pipelineInfo.layout = shader->NativeData.PipelineLayout;

			VK_CHECK(vkCreateComputePipelines(m_Device.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader->NativeData.Pipeline));
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
			builder.DisableBlending();
			builder.SetColorAttachments(colorAttachmentFormats.data(), (uint32_t)colorAttachmentFormats.size());

			if (desc.EnableDepthTest) {

				builder.SetDepthFormat(VK_FORMAT_D32_SFLOAT);
				builder.EnableDepthTest(desc.EnableDepthWrite, VK_COMPARE_OP_GREATER_OR_EQUAL);
			}
			else {
				builder.DisableDepthTest();
			}

			builder.PipelineLayout = shader->NativeData.PipelineLayout;
			shader->NativeData.Pipeline = builder.BuildPipeline(m_Device.Device);

			vkDestroyShaderModule(m_Device.Device, vertexModule, nullptr);
			vkDestroyShaderModule(m_Device.Device, pixelModule, nullptr);
		}

		return shader;
	}

	void VulkanRHIDevice::DestroyShaderRHI(RHIData* data) {

		VulkanRHIShader::Data* vkShader = (VulkanRHIShader::Data*)data->GetNativeData();

		if (vkShader->PipelineLayout) {
			vkDestroyPipelineLayout(m_Device.Device, vkShader->PipelineLayout, nullptr);
		}

		if (vkShader->Pipeline) {
			vkDestroyPipeline(m_Device.Device, vkShader->Pipeline, nullptr);
		}

		delete data;
	}

	void VulkanRHIDevice::BindShader(RHICommandBuffer* cmd, RHIShader* shader, std::vector<RHIBindingSet*> shaderSets, void* pushData) {

		VulkanRHIShader::Data* vkShader = (VulkanRHIShader::Data*)shader->GetRHIData()->GetNativeData();
		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

		VkPipelineBindPoint bindPoint = shader->GetShaderType() == EShaderType::ECompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkCmdBindPipeline(vkCmd->Cmd, bindPoint, vkShader->Pipeline);

		for (int i = 0; i < shaderSets.size(); i++) {

			RHIBindingSet* set = shaderSets[i];
			VulkanRHIBindingSet::Data* vkSet = (VulkanRHIBindingSet::Data*)set->GetRHIData()->GetNativeData();

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

						VulkanRHITextureView::Data* vkView = (VulkanRHITextureView::Data*)w.Texture->GetRHIData()->GetNativeData();

						VkDescriptorImageInfo& info = imageInfos.emplace_back(
							VkDescriptorImageInfo{
							.sampler = nullptr,
							.imageView = vkView->View,
							.imageLayout = VulkanUtils::GPUAccessToVulkanLayout(w.TextureAccess)
							});
						write.pImageInfo = &info;
					}
					else if (w.Buffer) {

						VulkanRHIBuffer::Data* vkBuff = (VulkanRHIBuffer::Data*)w.Buffer->GetRHIData()->GetNativeData();

						VkDescriptorBufferInfo& info = bufferInfos.emplace_back(
							VkDescriptorBufferInfo{
							.buffer = vkBuff->Buffer,
							.offset = w.BufferOffset,
							.range = w.BufferRange
							});
						write.pBufferInfo = &info;
					}
					else if (w.Sampler) {

						VulkanRHISampler::Data* vkSampler = (VulkanRHISampler::Data*)w.Sampler->GetRHIData()->GetNativeData();

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

	RHIData* VulkanRHIDevice::CreateSamplerRHI(const SamplerDesc& desc) {

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
		createInfo.anisotropyEnable = desc.EnableAnisotropy;
		createInfo.maxAnisotropy = desc.MaxAnisotropy;

		VkSamplerReductionModeCreateInfoEXT createInfoReduction{};
		if (desc.Reduction != ESamplerReduction::ENone) {

			createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;
			createInfoReduction.reductionMode = VulkanUtils::SamplerReductionToVulkan(desc.Reduction);
			createInfo.pNext = &createInfoReduction;
		}

		VK_CHECK(vkCreateSampler(m_Device.Device, &createInfo, 0, &sampler->NativeData.Sampler));
		return sampler;
	}

	void VulkanRHIDevice::DestroySamplerRHI(RHIData* data) {

		VulkanRHISampler::Data* vkSampler = (VulkanRHISampler::Data*)data->GetNativeData();

		if (vkSampler->Sampler) {
			vkDestroySampler(m_Device.Device, vkSampler->Sampler, nullptr);
		}

		delete data;
	}

	ImTextureID VulkanRHIDevice::CreateStaticGuiTexture(RHITextureView* texture) {

		VulkanRHITextureView::Data* vkView = (VulkanRHITextureView::Data*)texture->GetRHIData()->GetNativeData();
		VulkanRHISampler::Data* vkSampler = (VulkanRHISampler::Data*)texture->GetSourceTexture()->GetSampler()->GetRHIData()->GetNativeData();

		return m_GuiTextureManager.CreateStaticGuiTexture(vkView->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkSampler->Sampler);
	}

	void VulkanRHIDevice::DestroyStaticGuiTexture(ImTextureID id) {
		m_GuiTextureManager.DestroyStaticGuiTexture(id);
	}

	ImTextureID VulkanRHIDevice::CreateDynamicGuiTexture(RHITextureView* texture) {

		VulkanRHITextureView::Data* vkView = (VulkanRHITextureView::Data*)texture->GetRHIData()->GetNativeData();
		VulkanRHISampler::Data* vkSampler = (VulkanRHISampler::Data*)texture->GetSourceTexture()->GetSampler()->GetRHIData()->GetNativeData();

		uint32_t frameIndex = GFrameRenderer->GetFrameCount() % 2;
		return m_GuiTextureManager.CreateDynamicGuiTexture(vkView->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkSampler->Sampler, frameIndex);
	}

	RHIData* VulkanRHIDevice::CreateCommandBufferRHI() {

		VulkanRHICommandBuffer* cmd = new VulkanRHICommandBuffer();

		VkCommandPoolCreateInfo commandPoolInfo = VulkanUtils::CommandPoolCreateInfo(m_Device.Queues.GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VK_CHECK(vkCreateCommandPool(m_Device.Device, &commandPoolInfo, nullptr, &cmd->NativeData.Pool));

		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanUtils::CommandBufferAllocInfo(cmd->NativeData.Pool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_Device.Device, &cmdAllocInfo, &cmd->NativeData.Cmd));

		return cmd;
	}

	void VulkanRHIDevice::DestroyCommandBufferRHI(RHIData* data) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)data->GetNativeData();

		if (vkCmd->Pool) {
			vkDestroyCommandPool(m_Device.Device, vkCmd->Pool, nullptr);
		}

		delete data;
	}

	void VulkanRHIDevice::BeginFrameCommandBuffer(RHICommandBuffer* cmd) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanUtils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(vkCmd->Cmd, &cmdBeginInfo));
	}

	void VulkanRHIDevice::WaitForFrameCommandBuffer(RHICommandBuffer* cmd) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

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

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)m_ImmCmd->GetRHIData()->GetNativeData();

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

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		vkCmdDispatch(vkCmd->Cmd, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanRHIDevice::WaitGPUIdle() {

		vkDeviceWaitIdle(m_Device.Device);
	}

	void VulkanRHIDevice::BeginRendering(RHICommandBuffer* cmd, const RenderInfo& info) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();

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

			VulkanRHITextureView::Data* vkView = (VulkanRHITextureView::Data*)v->GetRHIData()->GetNativeData();
			colorAttachments.push_back(VulkanUtils::AttachmentInfo(vkView->View, info.ColorClear ? &colorClear : nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
		}

		VkRenderingAttachmentInfo depthAttachment{};
		if (info.DepthTarget) {

			VulkanRHITextureView::Data* vkView = (VulkanRHITextureView::Data*)info.DepthTarget->GetRHIData()->GetNativeData();
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

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		vkCmdEndRendering(vkCmd->Cmd);
	}

	void VulkanRHIDevice::DrawIndirectCount(RHICommandBuffer* cmd, RHIBuffer* commBuffer, size_t offset, RHIBuffer* countBuffer,
		size_t countBufferOffset, uint32_t maxDrawCount, uint32_t commStride) 
	{
		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		VulkanRHIBuffer::Data* vkCommBuff = (VulkanRHIBuffer::Data*)commBuffer->GetRHIData()->GetNativeData();
		VulkanRHIBuffer::Data* vkCountBuff = (VulkanRHIBuffer::Data*)countBuffer->GetRHIData()->GetNativeData();

		vkCmdDrawIndirectCount(vkCmd->Cmd, vkCommBuff->Buffer, offset, vkCountBuff->Buffer, 
			countBufferOffset, maxDrawCount, commStride);

		//vkCmdDrawIndirect(vkCmd->Cmd, vkCommBuff->Buffer, offset, 1, commStride);
	}

	void VulkanRHIDevice::Draw(RHICommandBuffer* cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		vkCmdDraw(vkCmd->Cmd, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanRHIDevice::BeginGuiNewFrame() {

		m_GuiTextureManager.DynamicTexSetsIndex = 0;
		ImGui_ImplVulkan_NewFrame();
	}

	void VulkanRHIDevice::DrawSwapchain(RHICommandBuffer* cmd, uint32_t width, uint32_t height, bool drawGui, RHITexture2D* fillTexture) {

		VulkanRHICommandBuffer::Data* vkCmd = (VulkanRHICommandBuffer::Data*)cmd->GetRHIData()->GetNativeData();
		uint32_t frameIndex = GFrameRenderer->GetFrameCount() % 2;

		uint32_t swapchainImageIndex = 0;
		VkResult check = vkAcquireNextImageKHR(m_Device.Device, m_Swapchain.Swapchain, 1000000000, m_SyncObjects[frameIndex].SwapchainSemaphore, nullptr, &swapchainImageIndex);
		if (check == VK_ERROR_OUT_OF_DATE_KHR) {
			m_Swapchain.Resize(m_Device, width, height);
			return;
		}

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
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCmd->Cmd);
			vkCmdEndRendering(vkCmd->Cmd);
			};

		if (fillTexture) {

			VulkanRHITexture::Data* vkFillTex = (VulkanRHITexture::Data*)fillTexture->GetRHIData()->GetNativeData();

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


	void VulkanImGuiTextureManager::Init(uint32_t numDynamicTextures) {

		for (int f = 0; f < 2; f++) {

			DynamicTexSets[f].reserve(numDynamicTextures);

			for (uint32_t t = 0; t < numDynamicTextures; t++) {

				VkDescriptorSet newSet = ImGui_ImplVulkan_CreateTextureDescSet();
				DynamicTexSets[f].push_back(newSet);
			}
		}
	}

	void VulkanImGuiTextureManager::Cleanup() {

		for (int f = 0; f < 2; f++) {

			for (int i = 0; i < DynamicTexSets[f].size(); i++) {

				ImGui_ImplVulkan_RemoveTexture(DynamicTexSets[f][i]);
			}

			DynamicTexSets[f].clear();
		}
	}

	ImTextureID VulkanImGuiTextureManager::CreateStaticGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler) {

		VkDescriptorSet texSet = ImGui_ImplVulkan_AddTexture(sampler, view, layout);
		return (ImTextureID)texSet;
	}

	void VulkanImGuiTextureManager::DestroyStaticGuiTexture(ImTextureID id) {

		VkDescriptorSet texSet = (VkDescriptorSet)id;
		ImGui_ImplVulkan_RemoveTexture(texSet);
	}

	ImTextureID VulkanImGuiTextureManager::CreateDynamicGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler, uint32_t currentFrameIndex) {

		VkDescriptorSet dynamicTexSet = DynamicTexSets[currentFrameIndex][DynamicTexSetsIndex];
		DynamicTexSetsIndex++;

		ImGui_ImplVulkan_WriteTextureDescSet(sampler, view, layout, dynamicTexSet);

		return (ImTextureID)dynamicTexSet;
	}
}