#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/Shader.h>

Spike::SamplerCache* Spike::GSamplerCache = nullptr;

namespace Spike {

	void RHITextureView::InitRHI() {

		m_RHIData = GRHIDevice->CreateTextureViewRHI(m_Desc);

		bool isStorage = EnumHasAllFlags(m_Desc.SourceTexture->GetUsageFlags(), ETextureUsageFlags::EStorage);
		bool isSampled = EnumHasAllFlags(m_Desc.SourceTexture->GetUsageFlags(), ETextureUsageFlags::ESampled);

		if (isSampled)
			m_SRVIndex = GShaderManager->GetTextureSRVIndex(this, false);
		if (isStorage)
			m_UAVIndex = GShaderManager->GetTextureUAVIndex(this);
		if (isSampled && isStorage)
			m_UAVReadOnlyIndex = GShaderManager->GetTextureSRVIndex(this, true);
	}

	void RHITextureView::ReleaseRHIImmediate() {

		GRHIDevice->DestroyTextureViewRHI(m_RHIData);

		ETextureType type = m_Desc.SourceTexture->GetTextureType();

		if (m_SRVIndex != INVALID_SHADER_INDEX)
			GShaderManager->ReleaseTextureSRVIndex(m_SRVIndex, type);
		if (m_UAVReadOnlyIndex != INVALID_SHADER_INDEX)
			GShaderManager->ReleaseTextureSRVIndex(m_UAVReadOnlyIndex, type);
		if (m_UAVIndex != INVALID_SHADER_INDEX)
			GShaderManager->ReleaseTextureUAVIndex(m_UAVIndex, type, m_Desc.SourceTexture->GetFormat());
	}

	void RHITextureView::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([data = m_RHIData, type = m_Desc.SourceTexture->GetTextureType(), format = m_Desc.SourceTexture->GetFormat(), srv = m_SRVIndex, uavReadOnly = m_UAVReadOnlyIndex, uav = m_UAVIndex]() {

			GRHIDevice->DestroyTextureViewRHI(data);

			if (srv != INVALID_SHADER_INDEX)
				GShaderManager->ReleaseTextureSRVIndex(srv, type);
			if (uavReadOnly != INVALID_SHADER_INDEX)
				GShaderManager->ReleaseTextureSRVIndex(uavReadOnly, type);
			if (uav != INVALID_SHADER_INDEX)
				GShaderManager->ReleaseTextureUAVIndex(uav, type, format);
			});
	}

	void RHISampler::InitRHI() {

		m_RHIData = GRHIDevice->CreateSamplerRHI(m_Desc);
		m_ShaderIndex = GShaderManager->GetSamplerIndex(this);
	}

	void RHISampler::ReleaseRHI() {

		GRHIDevice->DestroySamplerRHI(m_RHIData);
		GShaderManager->ReleaseSamplerIndex(m_ShaderIndex);
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