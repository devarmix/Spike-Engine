#include <Engine/Renderer/CubeTexture.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>

#include <stb_image.h>

namespace Spike {

	void CubeTextureResource::InitGPUData() {

		if (m_FilterMode == EFilterNone) {

			m_GPUData = GGfxDevice->CreateCubeTextureGPUData(m_Size, m_Format, m_UsageFlags, m_SamplerTexture);
		}
		else {

			m_GPUData = GGfxDevice->CreateFilteredCubeTextureGPUData(m_Size, m_Format, m_UsageFlags, m_SamplerTexture, m_FilterMode);
		}
	}

	void CubeTextureResource::ReleaseGPUData() {

		GGfxDevice->DestroyCubeTextureGPUData(m_GPUData);
	}
}

namespace SpikeEngine {

	CubeTexture::CubeTexture(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode) {

		CreateResource(size, format, usageFlags, samplerTexture, filterMode);
	}

	CubeTexture::~CubeTexture() {

		ReleaseResource();
		ASSET_CORE_DESTROY();
	}

	Ref<CubeTexture> CubeTexture::Create(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, Ref<Texture2D> samplerTexture) {

		return CreateRef<CubeTexture>(size, format, usageFlags, samplerTexture->GetResource(), EFilterNone);
	}

	Ref<CubeTexture> CubeTexture::Create(const char* filePath, uint32_t size, ETextureFormat format) {

		Ref<Texture2D> samplerTexture = Texture2D::Create(filePath, format);

		return CubeTexture::Create(size, format, EUsageFlagSampled | EUsageFlagStorage, samplerTexture);
	}

	Ref<CubeTexture> CubeTexture::CreateFiltered(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, Ref<CubeTexture> samplerTexture, ECubeTextureFilterMode filterMode) {

		return CreateRef<CubeTexture>(size, format, usageFlags, samplerTexture->GetResource(), filterMode);
	}

	void CubeTexture::CreateResource(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode) {

		m_RenderResource = new CubeTextureResource(size, format, usageFlags, samplerTexture, filterMode);
		SafeRenderResourceInit(m_RenderResource);
	}

	void CubeTexture::ReleaseResource() {

		SafeRenderResourceRelease(m_RenderResource);
		m_RenderResource = nullptr;
	}
}