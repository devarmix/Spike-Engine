#include <Platforms/Vulkan/VulkanGui.h>
#include <Platforms/Vulkan/VulkanTexture.h>
#include <Platforms/Vulkan/VulkanRenderer.h>
#include "imgui_impl_vulkan.h"

namespace Spike {

	VulkanImGuiTextureMapper* VulkanImGuiTextureMapper::Create() {

		return new VulkanImGuiTextureMapper();
	}

	void VulkanImGuiTextureMapper::Cleanup() {

		for (auto& [k, v] : m_TextureMap) {

			ImGui_ImplVulkan_RemoveTexture(v);
		}

		m_TextureMap.clear();
	}

	ImTextureID VulkanImGuiTextureMapper::RegisterTexture(Ref<Texture> texture) {

		const VulkanTextureData* textureData = (const VulkanTextureData*)texture->GetData();
		VkDescriptorSet textureSet = ImGui_ImplVulkan_AddTexture(VulkanRenderer::DefSamplerLinear, textureData->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		ImTextureID textureID = (ImTextureID)textureSet;
		m_TextureMap[(uint64_t)textureID] = textureSet;

		return textureID;
	}

	void VulkanImGuiTextureMapper::UnregisterTexture(ImTextureID id) {

		auto it = m_TextureMap.find((uint64_t)id);
		if (it != m_TextureMap.end()) {

			vkDeviceWaitIdle(VulkanRenderer::Device.Device);

			VkDescriptorSet textureSet = m_TextureMap[(uint64_t)id];
			ImGui_ImplVulkan_RemoveTexture(textureSet);

			m_TextureMap.erase((uint64_t)id);
		} 
		else {

			ENGINE_ERROR("Trying to unregister a texture than wasnt registered!");
		}
	}

	void VulkanImGuiTextureMapper::UpdateTexture(ImTextureID id, Ref<Texture> newTexture) {

		auto it = m_TextureMap.find((uint64_t)id);
		if (it != m_TextureMap.end()) {

			vkDeviceWaitIdle(VulkanRenderer::Device.Device);

			VkDescriptorSet textureSet = m_TextureMap[(uint64_t)id];

			const VulkanTextureData* textureData = (const VulkanTextureData*)newTexture->GetData();
			ImGui_ImplVulkan_UpdateTexture(textureSet, VulkanRenderer::DefSamplerLinear, textureData->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else {

			ENGINE_ERROR("Trying to update a texture that wasnt registered!");
		}
	}
}