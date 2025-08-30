#include <Backends/Vulkan/VulkanSwapchain.h>

#include <VkBootstrap.h>

namespace Spike {

	VulkanSwapchain::VulkanSwapchain() :
		Swapchain(nullptr),
		Format(VK_FORMAT_UNDEFINED),
		Extent(0, 0) {}

	void VulkanSwapchain::Init(VulkanDevice& device, uint32_t width, uint32_t height) {

		vkb::SwapchainBuilder swapchainBuilder{ 
			device.PhysicalDevice, 
			device.Device,
			device.Surface };

		Format = VK_FORMAT_R8G8B8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = Format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		Extent = vkbSwapchain.extent;

		Swapchain = vkbSwapchain.swapchain;
		Images = vkbSwapchain.get_images().value();
		ImageViews = vkbSwapchain.get_image_views().value();
	}

	void VulkanSwapchain::Resize(VulkanDevice& device, uint32_t width, uint32_t height) {

		//vkDeviceWaitIdle(device->Device);

		Destroy(device);
		Init(device, width, height);
	}

	void VulkanSwapchain::Destroy(VulkanDevice& device) {

		if (Swapchain) {

			vkDestroySwapchainKHR(device.Device, Swapchain, nullptr);
			Swapchain = nullptr;
		}

		// destroy swapchain resources
		for (int i = 0; i < ImageViews.size(); i++) {

			if (ImageViews[i]) {

				vkDestroyImageView(device.Device, ImageViews[i], nullptr);
				ImageViews[i] = nullptr;
			}
		}
	}
}