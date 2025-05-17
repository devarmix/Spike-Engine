#include <Platforms/Vulkan/VulkanDevice.h>

#include <VkBootstrap.h>
#include <SDL_vulkan.h>

namespace Spike {

	void VulkanDevice::Init(Window& window, bool useValidationLayers) {

		vkb::InstanceBuilder builder;

		// make vulkan instance, with basic textures
		auto inst_ret = builder.set_app_name(window.GetName().c_str())
			.request_validation_layers(useValidationLayers)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();

		vkb::Instance vkb_inst = inst_ret.value();

		// grab the instance
		Instance = vkb_inst.instance;
		DebugMessenger = vkb_inst.debug_messenger;

		SDL_Vulkan_CreateSurface((SDL_Window*)window.GetNativeWindow(), Instance, &Surface);

		// vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features.dynamicRendering = true;
		features.synchronization2 = true;

		// vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		features12.shaderSampledImageArrayNonUniformIndexing = true;
		features12.shaderStorageBufferArrayNonUniformIndexing = true;

		features12.runtimeDescriptorArray = true;
		features12.descriptorBindingVariableDescriptorCount = true;
		features12.descriptorBindingPartiallyBound = true;

		features12.descriptorBindingSampledImageUpdateAfterBind = true;
		features12.descriptorBindingStorageBufferUpdateAfterBind = true;

		// select the gpu, which supports vulkan 1.3 and can write to the SDL surface
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(Surface)
			.select()
			.value();

		// create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// get the VkDevice handle to be used in the rest of the application
		Device = vkbDevice.device;
		PhysicalDevice = physicalDevice.physical_device;

		// get graphics queues
		Queues.GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		Queues.GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		Queues.ComputeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
		Queues.ComputeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

		// initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &Allocator);
	}

	void VulkanDevice::Destroy() {

		if (Allocator != VK_NULL_HANDLE) {

			vmaDestroyAllocator(Allocator);
			Allocator = VK_NULL_HANDLE;
		}

		if (Surface != VK_NULL_HANDLE) {

			vkDestroySurfaceKHR(Instance, Surface, nullptr);
			Surface = VK_NULL_HANDLE;
		}

		if (Device != VK_NULL_HANDLE) {

			vkDestroyDevice(Device, nullptr);
			Device = VK_NULL_HANDLE;
		}

		if (DebugMessenger != VK_NULL_HANDLE) {

			vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
			DebugMessenger = VK_NULL_HANDLE;
		}

		if (Instance != VK_NULL_HANDLE) {

			vkDestroyInstance(Instance, nullptr);
			Instance = VK_NULL_HANDLE;
		}
	}
}