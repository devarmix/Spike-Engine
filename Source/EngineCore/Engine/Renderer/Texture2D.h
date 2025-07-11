#pragma once

#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/TextureBase.h>
using namespace Spike;

namespace Spike {

	class Texture2DResource : public TextureResource {
	public:

		Texture2DResource(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData, bool needCPUData)
			: TextureResource(format, usageFlags), m_Width(width), m_Height(height), m_Mipmaped(mipmap), m_PixelData(pixelData), m_NeedCPUData(needCPUData) {}

		virtual ~Texture2DResource() override {}

		virtual void InitGPUData() override;
		virtual void ReleaseGPUData() override;

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		bool IsMipmaped() const { return m_Mipmaped; }

		void DeletePixelData();

	private:

		uint32_t m_Width;
		uint32_t m_Height;

		bool m_Mipmaped;

		// contains pixel data. can be null if not used
		void* m_PixelData;
		bool m_NeedCPUData;
	};
}

namespace SpikeEngine {

	class Texture2D : public Asset {
	public:

		Texture2D(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData, bool needCPUData);
		virtual ~Texture2D() override;

		static Ref<Texture2D> Create(const char* filePath, ETextureFormat format = EFormatRGBA8U);
		static Ref<Texture2D> Create(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap = false, void* pixelData = nullptr, bool needCPUData = false);

		Texture2DResource* GetResource() { return m_RenderResource; }
		void ReleaseResource();
		void CreateResource(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap = false, void* pixelData = nullptr, bool needCPUData = false);

		uint32_t GetWidth() const { return m_RenderResource->GetWidth(); }
		uint32_t GetHeight() const { return m_RenderResource->GetHeight(); }

		ETextureFormat GetFormat() const { return m_RenderResource->GetFormat(); }

		ASSET_CLASS_TYPE(Texture2DAsset)

	private:

		Texture2DResource* m_RenderResource;
	};
}