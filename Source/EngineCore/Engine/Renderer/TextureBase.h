#pragma once

#include <Engine/Renderer/RHIResource.h>
#include <Engine/Utils/MathUtils.h>
#include <Engine/Core/Core.h>
#include <Engine/Serialization/FileStream.h>

namespace Spike {

	enum class ETextureType : uint8_t {

		ENone = 0,
		E2D,
		ECube
	};

	enum class ETextureFormat : uint8_t {

		ENone = 0,
		ERGBA8U,
		EBGRA8U,
		ERGBA16F,
		ERGBA32F,
		ERGBBC1,
		ERGBABC3,
		ERGBABC6,
		ERGBC5,
		ED32F,
		ER32F,
		ERG16F,
		ERG8U,
		ER8U
	};

	uint32_t TextureFormatToSize(ETextureFormat format);
	uint32_t GetNumTextureMips(uint32_t width, uint32_t height);

	enum class ETextureUsageFlags : uint8_t { 

		ESampled         = BIT(0),
		ECopySrc         = BIT(1),
		ECopyDst         = BIT(2),
		EStorage         = BIT(3),
		EColorTarget     = BIT(4),
		EDepthTarget     = BIT(5)
	};
	ENUM_FLAGS_OPERATORS(ETextureUsageFlags)

	// forward declarations
	class RHITextureView;
	class RHISampler;

	class RHITexture : public RHIResource {
	public:
		virtual ETextureFormat GetFormat() const = 0;
		virtual ETextureUsageFlags GetUsageFlags() const = 0;
		virtual Vec3Uint GetSizeXYZ() const = 0;
		virtual uint32_t GetNumMips() const = 0;
		virtual ETextureType GetTextureType() const = 0;
		virtual RHISampler* GetSampler() = 0;

		bool IsMipmaped() const { return GetNumMips() > 1; }

		RHIData GetRHIData() const { return m_RHIData; }
		RHITextureView* GetTextureView() { return m_TextureView; }

	protected:

		RHIData m_RHIData;
		RHITextureView* m_TextureView;
	};

	struct TextureViewDesc {

		uint32_t BaseMip;
		uint32_t NumMips;
		uint32_t BaseArrayLayer;
		uint32_t NumArrayLayers;

		RHITexture* SourceTexture;
		ETextureType Type = ETextureType::ENone;

		bool operator==(const TextureViewDesc& other) const {

			return (BaseMip == other.BaseMip
				&& Type == other.Type
				&& BaseArrayLayer == other.BaseArrayLayer
				&& NumMips == other.NumMips
				&& NumArrayLayers == other.NumArrayLayers
				&& SourceTexture == other.SourceTexture);
		}
	};

	class RHITextureView : public RHIResource {
	public:
		RHITextureView(const TextureViewDesc& desc) : m_Desc(desc), m_RHIData(0), m_MaterialIndex(UINT32_MAX) {}
		virtual ~RHITextureView() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;
		virtual void ReleaseRHIImmediate() override;

		RHIData GetRHIData() const { return m_RHIData; }

		uint32_t GetNumMips() const { return m_Desc.NumMips; }
		uint32_t GetBaseMip() const { return m_Desc.BaseMip; }
		uint32_t GetBaseArrayLayer() const { return m_Desc.BaseArrayLayer; }
		uint32_t GetNumArrayLayers() const { return m_Desc.NumArrayLayers; }

		// None if use type of source texture
		ETextureType GetType() const { return m_Desc.Type; }
		RHITexture* GetSourceTexture() { return m_Desc.SourceTexture; }

		const TextureViewDesc& GetDesc() const { return m_Desc; }
		uint32_t GetMaterialIndex();

	private:

		RHIData m_RHIData;
		TextureViewDesc m_Desc;

		uint32_t m_MaterialIndex;
	};

	enum class ESamplerFilter : uint8_t {

		ENone = 0,
		EPoint,
		EBilinear,
		ETrilinear
	};

	enum class ESamplerAddress : uint8_t {

		ENone = 0,
		ERepeat,
		EClamp,
		EMirror
	};

	enum class ESamplerReduction : uint8_t {

		ENone = 0,
		EMinimum,
		EMaximum
	};

	struct SamplerDesc {

		ESamplerFilter Filter;

		ESamplerAddress AddressU;
		ESamplerAddress AddressV;
		ESamplerAddress AddressW;

		ESamplerReduction Reduction = ESamplerReduction::ENone;

		float MipLODBias = 0.f;
		float MinLOD = 0.f;
		float MaxLOD = 0.f;
		float MaxAnisotropy = 0.f;

		bool operator==(const SamplerDesc& other) const {

			return (Filter == other.Filter
				&& AddressU == other.AddressU
				&& AddressV == other.AddressV
				&& AddressW == other.AddressW
				&& Reduction == other.Reduction
				&& MipLODBias == other.MipLODBias
				&& MinLOD == other.MinLOD
				&& MaxLOD == other.MaxLOD
				&& MaxAnisotropy == other.MaxAnisotropy);
		}

		struct Hasher {

			size_t operator()(const SamplerDesc& desc) const {

				size_t h = std::hash<uint8_t>{}((uint8_t)desc.Filter);
				MathUtils::HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.AddressU));
				MathUtils::HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.AddressV));
				MathUtils::HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.AddressW));
				MathUtils::HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.Reduction));
				MathUtils::HashCombine(h, std::hash<float>{}(desc.MipLODBias));
				MathUtils::HashCombine(h, std::hash<float>{}(desc.MinLOD));
				MathUtils::HashCombine(h, std::hash<float>{}(desc.MaxLOD));
				MathUtils::HashCombine(h, std::hash<float>{}(desc.MaxAnisotropy));

				return h;
			}
		};
	};

	class RHISampler : public RHIResource {
	public:
		RHISampler(const SamplerDesc& desc) : m_Desc(desc), m_RHIData(0), m_MaterialIndex(UINT32_MAX) {}
		virtual ~RHISampler() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		const SamplerDesc& GetDesc() const { return m_Desc; }
		RHIData GetRHIData() const { return m_RHIData; }

		uint32_t GetMaterialIndex();

	private:

		SamplerDesc m_Desc;
		RHIData m_RHIData;

		uint32_t m_MaterialIndex;
	};

	class SamplerCache {
	public:
		SamplerCache() {}
		~SamplerCache() { Free(); }

		void Free();

		RHISampler* Get(const SamplerDesc& desc);

	private:

		std::unordered_map<SamplerDesc, RHISampler*, SamplerDesc::Hasher> m_Cache;
	};

	// global sampler cache pointer
	extern SamplerCache* GSamplerCache;
}