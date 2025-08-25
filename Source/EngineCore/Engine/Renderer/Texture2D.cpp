#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Log.h>

#include <stb_image.h>

uint32_t Spike::GetNumTextureMips(uint32_t width, uint32_t height) {

	return uint32_t(std::floor(std::log2(std::max(width, height)))) + 1;
}

namespace Spike {

	RHITexture2D::RHITexture2D(const Texture2DDesc& desc) : m_Desc(desc) {

		m_RHIData = nullptr;
		m_Sampler = nullptr;

		TextureViewDesc viewDesc{};
		viewDesc.BaseMip = 0;
		viewDesc.NumMips = desc.NumMips;
		viewDesc.BaseArrayLayer = 0;
		viewDesc.NumArrayLayers = 1;
		viewDesc.SourceTexture = this;

		m_TextureView = new RHITextureView(viewDesc);
	}

	void RHITexture2D::InitRHI() {

		m_RHIData = GRHIDevice->CreateTexture2DRHI(m_Desc);

		if (EnumHasAllFlags(m_Desc.UsageFlags, ETextureUsageFlags::ESampled)) {
			m_Sampler = GSamplerCache->Get(m_Desc.SamplerDesc);
		}

		if (!m_Desc.NeedCPUData) {

			DeletePixelData();
		}

		m_TextureView->InitRHI();
	}

	void RHITexture2D::ReleaseRHIImmediate() {

		GRHIDevice->DestroyTexture2DRHI(m_RHIData);

		DeletePixelData();
		m_TextureView->ReleaseRHIImmediate();
		delete m_TextureView;
	}

	void RHITexture2D::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([data = m_RHIData]() {
			GRHIDevice->DestroyTexture2DRHI(data);
			});

		DeletePixelData();
		m_TextureView->ReleaseRHI();
		delete m_TextureView;
	}

	void RHITexture2D::DeletePixelData() {

		if (m_Desc.PixelData) {

			free(m_Desc.PixelData);
			m_Desc.PixelData = nullptr;
		}
	}


	Texture2D::Texture2D(const Texture2DDesc& desc) {

		CreateResource(desc);
	}

	Texture2D::~Texture2D() {

		ReleaseResource();
		ASSET_CORE_DESTROY();
	}

	void Texture2D::CreateResource(const Texture2DDesc& desc) {

		m_RHIResource = new RHITexture2D(desc);
		SafeRHIResourceInit(m_RHIResource);
	}

	void Texture2D::ReleaseResource() {

		SafeRHIResourceRelease(m_RHIResource);
		m_RHIResource = nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const char* filePath, const SamplerDesc& samplerDesc, ETextureFormat format) {

		int width, height, nrChannels;

		Ref<Texture2D> texture = nullptr;
		void* data = nullptr;

		if (format == ETextureFormat::ERGBA32F) {
			data = stbi_loadf(filePath, &width, &height, &nrChannels, 4);
		}
		else {
			data = stbi_load(filePath, &width, &height, &nrChannels, 4);
		}
		
		if (data) {

			//texture = Create(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

			Texture2DDesc newDesc{};
			newDesc.Width = width;
			newDesc.Height = height;
			newDesc.NumMips = GetNumTextureMips(width, height);
			newDesc.Format = format;
			newDesc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopySrc | ETextureUsageFlags::ECopyDst;
			newDesc.SamplerDesc = samplerDesc;
			newDesc.PixelData = data;
			newDesc.NeedCPUData = false;

			texture = Create(newDesc);
		}
		else {

			ENGINE_ERROR("Failed to load texture from path: {}", filePath);

			// in case of fail, return default error texture, to prevent crash
			return GRHIDevice->DefErrorTexture2D;
		}

		return texture;
	}

	Ref<Texture2D> Texture2D::Create(const Texture2DDesc& desc) {

		return CreateRef<Texture2D>(desc);
	}
}