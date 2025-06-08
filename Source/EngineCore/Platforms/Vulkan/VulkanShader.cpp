#include <Platforms/Vulkan/VulkanShader.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

#include <fstream>

namespace Spike {

	static bool LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule) {

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
		if (vkCreateShaderModule(VulkanRenderer::Device.Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			return false;
		}

		*outShaderModule = shaderModule;
		return true;
	}

	VulkanShader* VulkanShader::Create(const char* vertexPath, const char* fragmentPath) {

		VulkanShader* shader = new VulkanShader();

		if (!(LoadShaderModule(fragmentPath, &shader->FragmentModule) && LoadShaderModule(vertexPath, &shader->VertexModule))) {

			ENGINE_ERROR("Failed to load shader modules from paths: {0}, {1}", vertexPath, fragmentPath);
			delete shader;

			return nullptr;
		}

		return shader;
	}

	void VulkanShader::Destroy() {

		if (VertexModule != VK_NULL_HANDLE) {

			vkDestroyShaderModule(VulkanRenderer::Device.Device, VertexModule, nullptr);
			VertexModule = VK_NULL_HANDLE;
		}

		if (FragmentModule != VK_NULL_HANDLE)  {
			 
			vkDestroyShaderModule(VulkanRenderer::Device.Device, FragmentModule, nullptr);
			FragmentModule = VK_NULL_HANDLE;
		}
	}
}