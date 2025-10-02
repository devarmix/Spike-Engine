#pragma once

#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/TextureBase.h>

namespace Spike {

	struct Texture2DDesc {

		uint32_t Width;
		uint32_t Height;
		uint32_t NumMips = 1;
		ETextureFormat Format;
		ETextureUsageFlags UsageFlags;

		bool AutoCreateSampler = true;

		SamplerDesc SamplerDesc;
		RHISampler* Sampler = nullptr;

		bool operator==(const Texture2DDesc& other) const {

			if (!(Width == other.Width
				&& Height == other.Height
				&& NumMips == other.NumMips
				&& Format == other.Format
				&& UsageFlags == other.UsageFlags)) return false;

			if (AutoCreateSampler) {
				if (!other.AutoCreateSampler || SamplerDesc != other.SamplerDesc) return false;
			}

			return true;
		}
	};

	constexpr char TEXTURE_2D_MAGIC[4] = { 'S', 'E', 'T', '2' };
	struct Texture2DHeader {

		uint32_t Width;
		uint32_t Height;
		ETextureFormat Format;

		ESamplerFilter Filter;
		ESamplerAddress AddressU;
		ESamplerAddress AddressV;

		size_t ByteSize;
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

	private:

		Texture2DDesc m_Desc;
	};

	class Texture2D : public Asset {
	public:
		Texture2D(const Texture2DDesc& desc, UUID id);
		virtual ~Texture2D() override;

		static Ref<Texture2D> Create(BinaryReadStream& stream, UUID id);

		RHITexture2D* GetResource() { return m_RHIResource; }
		void ReleaseResource();
		void CreateResource(const Texture2DDesc& desc);

		Vec3Uint GetSizeXYZ() const { return m_RHIResource->GetSizeXYZ(); }
		ETextureFormat GetFormat() const { return m_RHIResource->GetFormat(); }
		uint32_t GetNumMips() const { return m_RHIResource->GetNumMips(); }
		bool IsMipmapped() const { return m_RHIResource->IsMipmaped(); }

		ASSET_CLASS_TYPE(EAssetType::ETexture2D);

	private:
		RHITexture2D* m_RHIResource;
	};
}