#pragma once

#include <Backends/Vulkan/VulkanCommon.h>
#include <Engine/Core/Window.h>

namespace Spike {

	class VulkanDevice {
	public:
		VulkanDevice();
		void Init(Window* window, bool useValidationLayers = false);

		void Destroy();

		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VkSurfaceKHR Surface;

		struct {

			VkQueue GraphicsQueue;
			uint32_t GraphicsQueueFamily;
		} Queues;

		VmaAllocator Allocator;
	};
}