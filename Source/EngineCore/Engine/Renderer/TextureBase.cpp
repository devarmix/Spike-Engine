#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/Shader.h>

Spike::SamplerCache* Spike::GSamplerCache = nullptr;

uint32_t Spike::TextureFormatToSize(ETextureFormat format) {

	switch (format)
	{
	case Spike::ETextureFormat::ERGBBC1:
	case Spike::ETextureFormat::ERGBA16F:
	case Spike::ETextureFormat::ERGBA16U:
	case Spike::ETextureFormat::ERG32F:
		return 8;
	case Spike::ETextureFormat::ERGBA32F:
	case Spike::ETextureFormat::ERGBABC3:
	case Spike::ETextureFormat::ERGBC5:
	case Spike::ETextureFormat::ERGBABC6:
		return 16;
	case Spike::ETextureFormat::ERGBA8U:
	case Spike::ETextureFormat::ED32F:
	case Spike::ETextureFormat::ER32F:
	case Spike::ETextureFormat::ERG16F:
	case Spike::ETextureFormat::ERG16U:
		return 4;
	case Spike::ETextureFormat::ERG8U:
		return 2;
	case Spike::ETextureFormat::ER8U:
		return 1;
	default:
		return 0;
	}
}

uint32_t Spike::GetNumTextureMips(uint32_t width, uint32_t height) {
	return uint32_t(std::floor(std::log2(std::max(width, height)))) + 1;
}

namespace Spike {

	void RHITextureView::InitRHI() {

		m_RHIData = GRHIDevice->CreateTextureViewRHI(m_Desc);
	}

	void RHITextureView::ReleaseRHIImmediate() {

		GRHIDevice->DestroyTextureViewRHI(m_RHIData);

		if (m_MaterialIndex != INVALID_SHADER_INDEX) {
			GShaderManager->ReleaseMatTextureIndex(m_MaterialIndex);
		}
	}

	void RHITextureView::ReleaseRHI() {

		GFrameRenderer->SubmitToFrameQueue([data = m_RHIData, matIndex = m_MaterialIndex]() {
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