#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Engine/Core/Window.h>

namespace Spike {

	struct VulkanDevice {

		VulkanDevice() = default;
		void Init(Window& window, bool useValidationLayers = false);

		void Destroy();

		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VkSurfaceKHR Surface;

		struct {

			VkQueue GraphicsQueue;
			uint32_t GraphicsQueueFamily;

			VkQueue ComputeQueue;
			uint32_t ComputeQueueFamily;

		} Queues;

		VmaAllocator Allocator;
	};
}