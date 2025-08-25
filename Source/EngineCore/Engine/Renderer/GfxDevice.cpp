#include <Engine/Renderer/GfxDevice.h>
#include <Backends/Vulkan/VulkanGfxDevice.h>

#include <Engine/Core/Application.h>

Spike::RHIDevice* Spike::GRHIDevice = nullptr;

uint32_t Spike::RoundUpToPowerOfTwo(float value) {

	float valueBase = glm::log2(value);

	if (glm::fract(valueBase) == 0.0f) {
		return (uint32_t)value;
	}

	return (uint32_t)glm::pow(2.0f, glm::trunc(valueBase + 1.0f));
}

uint32_t Spike::GetComputeGroupCount(uint32_t threadCount, uint32_t groupSize) {

	return (threadCount + groupSize - 1) / groupSize;
}

namespace Spike {

	void RHICommandBuffer::InitRHI() {

		m_RHIData = GRHIDevice->CreateCommandBufferRHI();
	}

	void RHICommandBuffer::ReleaseRHI() {

		GRHIDevice->DestroyCommandBufferRHI(m_RHIData);
	}

	void RHIDevice::Create(Window* window, bool useImGui) {

		EXECUTE_ON_RENDER_THREAD([=]() {

			GRHIDevice = new VulkanRHIDevice(window, useImGui);
			});

		// init default data
		//uint32_t pink = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		//newDevice->DefErrorTexture2D = Texture2D::Create(1, 1, EFormatRGBA8Unorm, EUsageFlagSampled, false, (void*)&pink);
	}
}