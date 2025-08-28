#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/Shader.h>

Spike::SamplerCache* Spike::GSamplerCache = nullptr;

namespace Spike {

	void RHITextureView::InitRHI() {

		m_RHIData = GRHIDevice->CreateTextureViewRHI(m_Desc);
	}

	void RHITextureView::ReleaseRHIImmediate() {

		GRHIDevice->DestroyTextureViewRHI(m_RHIData);
	}

	void RHITextureView::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([data = m_RHIData, matIndex = m_MaterialIndex]() {
			GRHIDevice->DestroyTextureViewRHI(data);

			if (matIndex != INVALID_SHADER_INDEX) {
				GShaderManager->ReleaseMatTextureIndex(matIndex);
			}
			});
	}

	uint32_t RHITextureView::GetMaterialIndex() {

		if (m_MaterialIndex == INVALID_SHADER_INDEX) {
			m_MaterialIndex = GShaderManager->GetMatTextureIndex(this);
		}

		return m_MaterialIndex;
	}

	void RHISampler::InitRHI() {

		m_RHIData = GRHIDevice->CreateSamplerRHI(m_Desc);
	}

	void RHISampler::ReleaseRHI() {

		GRHIDevice->DestroySamplerRHI(m_RHIData);

		if (m_MaterialIndex != INVALID_SHADER_INDEX) {
			GShaderManager->ReleaseMatSamplerIndex(m_MaterialIndex);
		}
	}

	uint32_t RHISampler::GetMaterialIndex() {

		if (m_MaterialIndex == INVALID_SHADER_INDEX) {
			m_MaterialIndex = GShaderManager->GetMatSamplerIndex(this);
		}

		return m_MaterialIndex;
	}

	void SamplerCache::Free() {

		for (auto& [k, v] : m_Cache) {

			v->ReleaseRHI();
			delete v;
		}

		m_Cache.clear();
	}

	RHISampler* SamplerCache::Get(const SamplerDesc& desc) {

		RHISampler* out = nullptr;

		auto it = m_Cache.find(desc);
		if (it != m_Cache.end()) {

			out = it->second;
		}
		else {

			out = new RHISampler(desc);
			out->InitRHI();
			m_Cache[desc] = out;
		}

		return out;
	}
}