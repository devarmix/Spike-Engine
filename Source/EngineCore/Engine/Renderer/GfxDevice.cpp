#include <Engine/Renderer/GfxDevice.h>
#include <Platforms/Vulkan/VulkanGfxDevice.h>

#include <Engine/Core/Application.h>

Spike::GfxDevice* Spike::GGfxDevice = nullptr;

uint32_t Spike::RoundUpToPowerOfTwo(float value) {

	float valueBase = glm::log2(value);

	if (glm::fract(valueBase) == 0.0f) {
		return (uint32_t)value;
	}

	return (uint32_t)glm::pow(2.0f, glm::trunc(valueBase + 1.0f));
}

namespace Spike {
	
	void GBufferResource::InitGPUData() {

		ETextureUsageFlags textureFlags{};
		textureFlags |= EUsageFlagColorAttachment;
		textureFlags |= EUsageFlagSampled;

		m_AlbedoMapData = GGfxDevice->CreateTexture2DGPUData(m_Width, m_Height, EFormatRGBA16F, textureFlags, false, nullptr);
		m_MaterialMapData = GGfxDevice->CreateTexture2DGPUData(m_Width, m_Height, EFormatRGBA8U, textureFlags, false, nullptr);
		m_NormalMapData = GGfxDevice->CreateTexture2DGPUData(m_Width, m_Height, EFormatRGBA16F, textureFlags, false, nullptr);
		m_DepthMapData = GGfxDevice->CreateDepthMapGPUData(m_Width, m_Height, m_DepthPyramidSize);
		m_BloomMapData = GGfxDevice->CreateBloomMapGPUData(m_Width, m_Height);

		ETextureUsageFlags ssaoFlags{};
		ssaoFlags |= EUsageFlagSampled;
		ssaoFlags |= EUsageFlagStorage;

		m_SSAOMapData = GGfxDevice->CreateTexture2DGPUData(m_Width, m_Height, EFormatR32F, ssaoFlags, false, nullptr);
		m_SSAOBlurMapData = GGfxDevice->CreateTexture2DGPUData(m_Width, m_Height, EFormatR32F, ssaoFlags, false, nullptr);
	}

	void GBufferResource::ReleaseGPUData() {

		GGfxDevice->DestroyTexture2DGPUData(m_AlbedoMapData);
		GGfxDevice->DestroyTexture2DGPUData(m_MaterialMapData);
		GGfxDevice->DestroyTexture2DGPUData(m_NormalMapData);
		GGfxDevice->DestroyDepthMapGPUData(m_DepthMapData);
		GGfxDevice->DestroyBloomMapGPUData(m_BloomMapData);
		GGfxDevice->DestroyTexture2DGPUData(m_SSAOMapData);
		GGfxDevice->DestroyTexture2DGPUData(m_SSAOBlurMapData);
	}

	void GfxDevice::Create(Window* window, bool useImGui) {

		EXECUTE_ON_RENDER_THREAD([=]() {

			GGfxDevice = new VulkanGfxDevice(window, useImGui);
			});

		// init default data
		//uint32_t pink = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		//newDevice->DefErrorTexture2D = Texture2D::Create(1, 1, EFormatRGBA8Unorm, EUsageFlagSampled, false, (void*)&pink);
	}
}