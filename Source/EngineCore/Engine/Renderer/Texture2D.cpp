#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>

#include <stb_image.h>

namespace Spike {

	void Texture2DResource::InitGPUData() {

		m_GPUData = GGfxDevice->CreateTexture2DGPUData(m_Width, m_Height, m_Format, m_UsageFlags, m_Mipmaped, m_PixelData);

		if (!m_NeedCPUData) {

			DeletePixelData();
		}
	}

	void Texture2DResource::ReleaseGPUData() {

		GGfxDevice->DestroyTexture2DGPUData(m_GPUData);
	}

	void Texture2DResource::DeletePixelData() {

		if (m_PixelData) stbi_image_free(m_PixelData);
	}
}

namespace SpikeEngine {

	Texture2D::Texture2D(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData, bool needCPUData) {

		CreateResource(width, height, format, usageFlags, mipmap, pixelData, needCPUData);
	}

	Texture2D::~Texture2D() {

		ReleaseResource();
		ASSET_CORE_DESTROY();
	}

	void Texture2D::CreateResource(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData, bool needCPUData) {

		m_RenderResource = new Texture2DResource(width, height, format, usageFlags, mipmap, pixelData, needCPUData);
		SafeRenderResourceInit(m_RenderResource);
	}

	void Texture2D::ReleaseResource() {

		SafeRenderResourceRelease(m_RenderResource);
		m_RenderResource = nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const char* filePath, ETextureFormat format) {

		int width, height, nrChannels;

		Ref<Texture2D> texture = nullptr;
		void* data = nullptr;

		if (format == EFormatRGBA32SFloat) {
			data = stbi_loadf(filePath, &width, &height, &nrChannels, 4);
		}
		else {
			data = stbi_load(filePath, &width, &height, &nrChannels, 4);
		}
		
		if (data) {

			//texture = Create(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
			texture = Create(width, height, format, EUsageFlagSampled | EUsageFlagTransferDst | EUsageFlagTransferSrc, true, data);
		}
		else {

			ENGINE_ERROR("Failed to load texture from path: {}", filePath);

			// in case of fail, return default error texture, to prevent crash
			return GGfxDevice->DefErrorTexture2D;
		}

		return texture;
	}

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData, bool needCPUData) {

		return CreateRef<Texture2D>(width, height, format, usageFlags, mipmap, pixelData, needCPUData);
	}
}