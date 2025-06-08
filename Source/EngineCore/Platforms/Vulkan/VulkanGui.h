#pragma once

#include <Engine/Layers/ImGuiLayer.h>
#include <Platforms/Vulkan/VulkanCommon.h>

namespace Spike {

	class VulkanImGuiTextureMapper : public ImGuiTextureMapper {
	public:
		VulkanImGuiTextureMapper() { m_TextureMap.clear(); }
		virtual ~VulkanImGuiTextureMapper() override { Cleanup(); }

		static VulkanImGuiTextureMapper* Create();

		void Cleanup();

		virtual ImTextureID RegisterTexture(Ref<Texture> texture) override;
		virtual void UnregisterTexture(ImTextureID id) override;

		virtual void UpdateTexture(ImTextureID id, Ref<Texture> newTexture) override;

	private:
		std::map<uint64_t, VkDescriptorSet> m_TextureMap;
	};
}