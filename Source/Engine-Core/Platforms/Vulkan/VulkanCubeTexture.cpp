#include <Platforms/Vulkan/VulkanCubeTexture.h>
#include <Platforms/Vulkan/VulkanTools.h>
#include <Platforms/Vulkan/VulkanBuffer.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

#include <stb_image.h>

namespace Spike {

	Ref<VulkanCubeTexture> VulkanCubeTexture::Create(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {

		Ref<VulkanCubeTexture> newTexture = CreateRef<VulkanCubeTexture>();
		newTexture->Data.Format = format;
		newTexture->Data.Extent = size;

		VkImageCreateInfo img_info = VulkanTools::CubeImageCreateInfo(format, usage, size);
		if (mipmapped) {
			img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
		}

		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(VulkanRenderer::Device.Allocator, &img_info, &allocinfo, &newTexture->Data.Image, &newTexture->Data.Allocation, nullptr));

		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (format == VK_FORMAT_D32_SFLOAT) {
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		// build a image-view for the image
		VkImageViewCreateInfo view_info = VulkanTools::CubeImageviewCreateInfo(format, newTexture->Data.Image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		VK_CHECK(vkCreateImageView(VulkanRenderer::Device.Device, &view_info, nullptr, &newTexture->Data.View));

		return newTexture;
	}

	Ref<VulkanCubeTexture> VulkanCubeTexture::Create(void** data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {

		size_t sdata_size = size.depth * size.width * size.height * 4;
		size_t data_size = sdata_size * 6;

		VulkanBuffer uploadbuffer = VulkanBuffer::Create(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		unsigned char* mappedData = (unsigned char*)uploadbuffer.AllocationInfo.pMappedData;

		for (int i = 0; i < 6; i++) {

			memcpy(mappedData + i * sdata_size, data[i], sdata_size);
		}

		Ref<VulkanCubeTexture> newTexture = Create(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

		VulkanRenderer::ImmediateSubmit([&](VkCommandBuffer cmd) {

			TransitionImage(cmd, newTexture->Data.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			std::vector<VkBufferImageCopy> copyRegion(6);
			for (int i = 0; i < 6; i++) {

				copyRegion[i] = {};
				copyRegion[i].bufferOffset = i * sdata_size;
				copyRegion[i].bufferRowLength = 0;
				copyRegion[i].bufferImageHeight = 0;

				copyRegion[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion[i].imageSubresource.mipLevel = 0;
				copyRegion[i].imageSubresource.baseArrayLayer = i;
				copyRegion[i].imageSubresource.layerCount = 1;

				copyRegion[i].imageExtent = size;
			}

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadbuffer.Buffer, newTexture->Data.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, copyRegion.data());

			if (mipmapped) {
				GenerateMipmaps(cmd, newTexture->Data.Image, VkExtent2D{ newTexture->Data.Extent.width, newTexture->Data.Extent.height });
			}
			else {
				TransitionImage(cmd, newTexture->Data.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
			});

		uploadbuffer.Destroy();

		return newTexture;
	}

	Ref<VulkanCubeTexture> VulkanCubeTexture::Create(const char** filePath) {

		int width, height, nrChannels;

		Ref<VulkanCubeTexture> texture;

		void* cubeData[6];

		for (int i = 0; i < 6; i++) {

			unsigned char* data = stbi_load(filePath[i], &width, &height, &nrChannels, 4);
			if (data) {

				cubeData[i] = data;
			}
			else {

				ENGINE_ERROR("Failed to load cube texture face from path: {}", filePath[i]);

				// cleanup already loaded resources, to prevent memory leak
				for (int r = 0; r < i; r++) {

					stbi_image_free(cubeData[r]);
				}

				// in case of fail, return default error texture cube, to prevent crash
				return VulkanRenderer::DefErrorCubeTexture;
			}
		}

		VkExtent3D imagesize;
		imagesize.width = width;
		imagesize.height = height;
		imagesize.depth = 1;

		texture = Create(cubeData, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

		for (int i = 0; i < 6; i++) {

			stbi_image_free(cubeData[i]);
		}

		return texture;
	}

	void VulkanCubeTexture::Destroy() {

		if (Data.View != VK_NULL_HANDLE) {

			vkDestroyImageView(VulkanRenderer::Device.Device, Data.View, nullptr);
			Data.View = VK_NULL_HANDLE;
		}

		if (Data.Image != VK_NULL_HANDLE) {

			vmaDestroyImage(VulkanRenderer::Device.Allocator, Data.Image, Data.Allocation);
			Data.Image = VK_NULL_HANDLE;
		}
	}

	void VulkanCubeTexture::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags imageAspectFlags, VkImageLayout currentLayout, VkImageLayout newLayout) {

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

		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 6;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;

		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);
	}

	void VulkanCubeTexture::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {

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

	void VulkanCubeTexture::GenerateMipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize) {

		int mipLevels = int(std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) + 1;

		ENGINE_WARN("Generate mips: {}", mipLevels);

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

				std::vector<VkImageBlit2> blitRegion(6);

				for (int i = 0; i < 6; i++) {

					blitRegion[i] = { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

					blitRegion[i].srcOffsets[1].x = imageSize.width;
					blitRegion[i].srcOffsets[1].y = imageSize.height;
					blitRegion[i].srcOffsets[1].z = 1;

					blitRegion[i].dstOffsets[1].x = halfSize.width;
					blitRegion[i].dstOffsets[1].y = halfSize.height;
					blitRegion[i].dstOffsets[1].z = 1;

					blitRegion[i].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blitRegion[i].srcSubresource.baseArrayLayer = i;
					blitRegion[i].srcSubresource.layerCount = 1;
					blitRegion[i].srcSubresource.mipLevel = mip;

					blitRegion[i].dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blitRegion[i].dstSubresource.baseArrayLayer = i;
					blitRegion[i].dstSubresource.layerCount = 1;
					blitRegion[i].dstSubresource.mipLevel = mip + 1;
				}

				VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
				blitInfo.dstImage = image;
				blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				blitInfo.srcImage = image;
				blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				blitInfo.filter = VK_FILTER_LINEAR;
				blitInfo.regionCount = 6;
				blitInfo.pRegions = blitRegion.data();

				vkCmdBlitImage2(cmd, &blitInfo);

				imageSize = halfSize;
			}
		}

		// transition all mip levels into the final read_only layout
		TransitionImage(cmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}