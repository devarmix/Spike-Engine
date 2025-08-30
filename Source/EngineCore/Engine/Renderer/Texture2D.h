#pragma once

#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/TextureBase.h>

namespace Spike {

	// utility function
	uint32_t GetNumTextureMips(uint32_t width, uint32_t height);

	struct Texture2DDesc {

		uint32_t Width;
		uint32_t Height;
		uint32_t NumMips = 1;
		ETextureFormat Format;
		ETextureUsageFlags UsageFlags;

		bool NeedCPUData = false;
		bool AutoCreateSampler = true;
		void* PixelData = nullptr;

		SamplerDesc SamplerDesc;
		RHISampler* Sampler = nullptr;

		bool operator==(const Texture2DDesc& other) const {

			if (!(Width == other.Width
				&& Height == other.Height
				&& NumMips == other.NumMips
				&& Format == other.Format
				&& UsageFlags == other.UsageFlags
				&& NeedCPUData == other.NeedCPUData)) return false;

			if (AutoCreateSampler) {

				if (!other.AutoCreateSampler || SamplerDesc != other.SamplerDesc) return false;
			}

			return true;
		}
	};

	class RHITexture2D : public RHITexture {
	public:
		RHITexture2D(const Texture2DDesc& desc);
		virtual ~RHITexture2D() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;
		virtual void ReleaseRHIImmediate() override;

		virtual ETextureFormat GetFormat() const override { return m_Desc.Format; }
		virtual ETextureUsageFlags GetUsageFlags() const override { return m_Desc.UsageFlags; }
		virtual Vec3Uint GetSizeXYZ() const override { return Vec3(m_Desc.Width, m_Desc.Height, 1); }
		virtual uint32_t GetNumMips() const override { return m_Desc.NumMips; }
		virtual ETextureType GetTextureType() const override { return ETextureType::E2D; }
		virtual RHISampler* GetSampler() override { return m_Desc.Sampler; }

		const Texture2DDesc& GetDesc() { return m_Desc; }

		void DeletePixelData();

	private:

		Texture2DDesc m_Desc;
	};

	class Texture2D : public Asset {
	public:
		Texture2D(const Texture2DDesc& desc);
		virtual ~Texture2D() override;

		static Ref<Texture2D> Create(const char* filePath, const SamplerDesc& samplerDesc, ETextureFormat format = ETextureFormat::ERGBA8U);
		static Ref<Texture2D> Create(const Texture2DDesc& desc);

		RHITexture2D* GetResource() { return m_RHIResource; }
		void ReleaseResource();
		void CreateResource(const Texture2DDesc& desc);

		Vec3Uint GetSizeXYZ() const { return m_RHIResource->GetSizeXYZ(); }
		ETextureFormat GetFormat() const { return m_RHIResource->GetFormat(); }
		uint32_t GetNumMips() const { return m_RHIResource->GetNumMips(); }
		bool IsMipmapped() const { return m_RHIResource->IsMipmaped(); }

		//ASSET_CLASS_TYPE(Texture2DAsset)

	private:

		RHITexture2D* m_RHIResource;
	};
}