#include <Platforms/Vulkan/VulkanShader.h>
#include <Platforms/Vulkan/VulkanGfxDevice.h>

#include <fstream>

namespace Spike {

	static bool LoadShaderModule(VulkanDevice* device, const char* filePath, VkShaderModule* outShaderModule) {

		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			return false;
		}

		size_t fileSize = (size_t)file.tellg();

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read((char*)buffer.data(), fileSize);
		file.close();

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device->Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			return false;
		}

		*outShaderModule = shaderModule;
		return true;
	}

    VulkanShader::VulkanShader() : 
		FragmentModule(nullptr),
		VertexModule(nullptr) {}

	VulkanShader::VulkanShader(VulkanDevice* device, const char* vertexPath, const char* fragmentPath) {

		if (!(LoadShaderModule(device, fragmentPath, &FragmentModule) && LoadShaderModule(device, vertexPath, &VertexModule))) {

			ENGINE_ERROR("Failed to load shader modules from paths: {0}, {1}", vertexPath, fragmentPath);
		}
	}

	void VulkanShader::Destroy(VulkanDevice* device) {

		if (VertexModule != VK_NULL_HANDLE) {

			vkDestroyShaderModule(device->Device, VertexModule, nullptr);
			VertexModule = VK_NULL_HANDLE;
		}

		if (FragmentModule != VK_NULL_HANDLE)  {
			 
			vkDestroyShaderModule(device->Device, FragmentModule, nullptr);
			FragmentModule = VK_NULL_HANDLE;
		}
	}


	VulkanComputeShader::VulkanComputeShader() : ComputeModule(nullptr) {}

	VulkanComputeShader::VulkanComputeShader(VulkanDevice* device, const char* filePath) {

		if (!LoadShaderModule(device, filePath, &ComputeModule)) {

			ENGINE_ERROR("Failed to load compute shader module from path: {0}", filePath);
		}
	}

	void VulkanComputeShader::Destroy(VulkanDevice* device) {

		if (ComputeModule != VK_NULL_HANDLE) {

			vkDestroyShaderModule(device->Device, ComputeModule, nullptr);
			ComputeModule = VK_NULL_HANDLE;
		}
	}
}