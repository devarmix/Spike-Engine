#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Platforms/Vulkan/VulkanDevice.h>

namespace Spike {

	class VulkanSwapchain {
	public:
		VulkanSwapchain();

		void Init(VulkanDevice* device, uint32_t width, uint32_t height);
		void Resize(VulkanDevice* device, uint32_t width, uint32_t height);

		void Destroy(VulkanDevice* device);

		VkSwapchainKHR Swapchain;
		VkFormat Format;

		std::vector<VkImage> Images;
		std::vector<VkImageView> ImageViews;
		VkExtent2D Extent;
	};
}
