#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>

namespace Spike {

	struct VulkanSwapchain {

		VulkanSwapchain() = default;

		void Init(uint32_t width, uint32_t height);
		void Resize(uint32_t width, uint32_t height);

		void Destroy();

		VkSwapchainKHR Swapchain;
		VkFormat Format;

		std::vector<VkImage> Images;
		std::vector<VkImageView> ImageViews;
		VkExtent2D Extent;
	};
}
