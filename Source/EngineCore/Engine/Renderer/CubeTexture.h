#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/Texture2D.h>
using namespace Spike;

namespace Spike {

	enum ECubeTextureFilterMode {

		EFilterNone = 0,
		EFilterIrradiance,
		EFilterRadiance
	};

	class CubeTextureResource : public TextureResource {
	public:

		CubeTextureResource(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode)
			: TextureResource(format, usageFlags), m_Size(size), m_FilterMode(filterMode), m_SamplerTexture(samplerTexture) {}

		virtual ~CubeTextureResource() override {}

		virtual void InitGPUData() override;
		virtual void ReleaseGPUData() override;

		uint32_t GetSize() const { return m_Size; }
		ECubeTextureFilterMode GetFilterMode() const { return m_FilterMode; }

	private:

		uint32_t m_Size;
		ECubeTextureFilterMode m_FilterMode;

		TextureResource* m_SamplerTexture;
	};
}

namespace SpikeEngine {

	class CubeTexture : public Asset {
	public:

		CubeTexture(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode);
		virtual ~CubeTexture() override;

		static Ref<CubeTexture> Create(const char* filePath, uint32_t size, ETextureFormat format = EFormatRGBA32SFloat);
		static Ref<CubeTexture> Create(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, Ref<Texture2D> samplerTexture);
		static Ref<CubeTexture> CreateFiltered(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, Ref<CubeTexture> samplerTexture, ECubeTextureFilterMode filterMode);

		CubeTextureResource* GetResource() { return m_RenderResource; }
		void ReleaseResource();
		void CreateResource(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode = EFilterNone);

		uint32_t GetSize() const { return m_RenderResource->GetSize(); }

		ETextureFormat GetFormat() const { return m_RenderResource->GetFormat(); }
		ECubeTextureFilterMode GetFilterMode() const { return m_RenderResource->GetFilterMode(); }

		ASSET_CLASS_TYPE(CubeTextureAsset)

	private:

		CubeTextureResource* m_RenderResource;
	};
}