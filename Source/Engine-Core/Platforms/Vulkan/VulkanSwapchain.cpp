#include <Platforms/Vulkan/VulkanSwapchain.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

#include <VkBootstrap.h>

namespace Spike {

	void VulkanSwapchain::Init(uint32_t width, uint32_t height) {

		vkb::SwapchainBuilder swapchainBuilder{ VulkanRenderer::Device.PhysicalDevice, VulkanRenderer::Device.Device, 
			VulkanRenderer::Device.Surface };

		Format = VK_FORMAT_B8G8R8A8_UNORM;

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

	void VulkanSwapchain::Resize(uint32_t width, uint32_t height) {

		vkDeviceWaitIdle(VulkanRenderer::Device.Device);

		Destroy();

		Init(width, height);
	}

	void VulkanSwapchain::Destroy() {

		if (Swapchain != VK_NULL_HANDLE) {

			vkDestroySwapchainKHR(VulkanRenderer::Device.Device, Swapchain, nullptr);
			Swapchain = VK_NULL_HANDLE;
		}

		// destroy swapchain resources
		for (int i = 0; i < ImageViews.size(); i++) {

			if (ImageViews[i] != VK_NULL_HANDLE) {

				vkDestroyImageView(VulkanRenderer::Device.Device, ImageViews[i], nullptr);
				ImageViews[i] = VK_NULL_HANDLE;
			}
		}
	}
}