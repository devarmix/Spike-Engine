#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Platforms/Vulkan/VulkanDevice.h>
#include <Engine/Core/Core.h>

namespace Spike {

	class VulkanShader {
	public:
		VulkanShader();
		VulkanShader(VulkanDevice* device, const char* vertexPath, const char* fragmentPath);

		~VulkanShader() {}

		void Destroy(VulkanDevice* device);

	public:

		VkShaderModule VertexModule;
		VkShaderModule FragmentModule;
	};

	class VulkanComputeShader {
	public:

		VulkanComputeShader();
		VulkanComputeShader(VulkanDevice* device, const char* filePath);

		~VulkanComputeShader() {}

		void Destroy(VulkanDevice* device);

	public:

		VkShaderModule ComputeModule;
	};
}
