#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/Texture2D.h>

namespace Spike {

	enum class ECubeTextureFilterMode : uint8_t {

		ENone = 0,
		EIrradiance,
		ERadiance
	};

	class RHICubeTexture;

	struct CubeTextureDesc {

		uint32_t Size;
		uint32_t NumMips = 1;

		ETextureFormat Format;
		ETextureUsageFlags UsageFlags;
		ECubeTextureFilterMode FilterMode = ECubeTextureFilterMode::ENone;
		bool AutoCreateSampler = true;
		RHITexture* SamplerTexture;

		SamplerDesc SamplerDesc;
		RHISampler* Sampler = nullptr;

		bool operator==(const CubeTextureDesc& other) const {

			if (!(Size == other.Size
				&& FilterMode == other.FilterMode
				&& NumMips == other.NumMips
				&& Format == other.Format
				&& UsageFlags == other.UsageFlags)) return false;

			if (AutoCreateSampler) {

				if (!other.AutoCreateSampler || SamplerDesc != other.SamplerDesc) return false;
			}

			return true;
		}
	};

	class RHICubeTexture : public RHITexture {
	public:
		RHICubeTexture(const CubeTextureDesc& desc);
		virtual ~RHICubeTexture() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;
		virtual void ReleaseRHIImmediate() override;

		virtual ETextureFormat GetFormat() const override { return m_Desc.Format; }
		virtual ETextureUsageFlags GetUsageFlags() const override { return m_Desc.UsageFlags; }
		virtual Vec3Uint GetSizeXYZ() const override { return Vec3(m_Desc.Size, m_Desc.Size, 1); }
		virtual uint32_t GetNumMips() const override { return m_Desc.NumMips; }
		virtual ETextureType GetTextureType() const override { return ETextureType::ECube; }
		virtual RHISampler* GetSampler() override { return m_Desc.Sampler; }

		ECubeTextureFilterMode GetFilterMode() const { return m_Desc.FilterMode; }
		const CubeTextureDesc& GetDesc() { return m_Desc; }

	private:

		CubeTextureDesc m_Desc;
	};

	class CubeTexture : public Asset {
	public:
		CubeTexture(const CubeTextureDesc& desc);
		virtual ~CubeTexture() override;

		static Ref<CubeTexture> Create(const char* filePath, uint32_t size, const SamplerDesc& samplerDesc, ETextureFormat format = ETextureFormat::ERGBA32F);
		static Ref<CubeTexture> Create(const CubeTextureDesc& desc);

		RHICubeTexture* GetResource() { return m_RHIResource; }
		void ReleaseResource();
		void CreateResource(const CubeTextureDesc& desc);

		Vec3Uint GetSizeXYZ() const { return m_RHIResource->GetSizeXYZ(); }
		ETextureFormat GetFormat() const { return m_RHIResource->GetFormat(); }
		ECubeTextureFilterMode GetFilterMode() const { return m_RHIResource->GetFilterMode(); }
		uint32_t GetNumMips() const { return m_RHIResource->GetNumMips(); }
		bool IsMipmapped() const { return m_RHIResource->IsMipmaped(); }

		//ASSET_CLASS_TYPE(CubeTextureAsset)

	private:

		RHICubeTexture* m_RHIResource;
	};
}