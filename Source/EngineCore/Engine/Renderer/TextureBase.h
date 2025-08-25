#pragma once

#include <Engine/Renderer/RHIResource.h>
#include <Engine/Core/Core.h>

namespace Spike {

	enum class ETextureType : uint8_t {

		ENone = 0,
		E2D,
		ECube
	};

	enum class ETextureFormat : uint8_t {

		ENone = 0,
		ERGBA8U,
		ERGBA16F,
		ERGBA32F,
		ED32F,
		ER32F,
		ERG16F
	};

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

		bool IsMipmaped() const { return GetNumMips() > 1; }

		RHIData* GetRHIData() { return m_RHIData; }
		RHITextureView* GetTextureView() { return m_TextureView; }
		RHISampler* GetSampler() { return m_Sampler; }

	protected:

		RHIData* m_RHIData;
		RHITextureView* m_TextureView;
		RHISampler* m_Sampler;
	};

	struct TextureViewDesc {

		uint32_t BaseMip;
		uint32_t NumMips;
		uint32_t BaseArrayLayer;
		uint32_t NumArrayLayers;

		RHITexture* SourceTexture;

		bool operator==(const TextureViewDesc& other) const {

			return (BaseMip == other.BaseMip
				&& BaseArrayLayer == other.BaseArrayLayer
				&& NumMips == other.NumMips
				&& NumArrayLayers == other.NumArrayLayers
				&& SourceTexture == other.SourceTexture);
		}
	};

	class RHITextureView : public RHIResource {
	public:
		RHITextureView(const TextureViewDesc& desc) : m_Desc(desc), m_RHIData(nullptr), m_SRVIndex(UINT32_MAX), m_UAVIndex(UINT32_MAX), m_UAVReadOnlyIndex(UINT32_MAX) {}
		virtual ~RHITextureView() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;
		virtual void ReleaseRHIImmediate() override;

		RHIData* GetRHIData() { return m_RHIData; }

		uint32_t GetNumMips() const { return m_Desc.NumMips; }
		uint32_t GetBaseMip() const { return m_Desc.BaseMip; }
		uint32_t GetBaseArrayLayer() const { return m_Desc.BaseArrayLayer; }
		uint32_t GetNumArrayLayers() const { return m_Desc.NumArrayLayers; }
		RHITexture* GetSourceTexture() { return m_Desc.SourceTexture; }

		const TextureViewDesc& GetDesc() const { return m_Desc; }
		uint32_t GetSRVIndex() const { return m_SRVIndex; }
		uint32_t GetUAVIndex() const { return m_UAVIndex; }
		uint32_t GetUAVReadOnlyIndex() const { return m_UAVReadOnlyIndex; }

	private:

		RHIData* m_RHIData;
		TextureViewDesc m_Desc;

		uint32_t m_SRVIndex;
		uint32_t m_UAVIndex;
		uint32_t m_UAVReadOnlyIndex;
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
		bool EnableAnisotropy = false;

		bool operator==(const SamplerDesc& other) const {

			return (Filter == other.Filter
				&& AddressU == other.AddressU
				&& AddressV == other.AddressV
				&& AddressW == other.AddressW
				&& Reduction == other.Reduction
				&& MipLODBias == other.MipLODBias
				&& MinLOD == other.MinLOD
				&& MaxLOD == other.MaxLOD
				&& MaxAnisotropy == other.MaxAnisotropy
				&& EnableAnisotropy == other.EnableAnisotropy);
		}

		struct Hasher {

			size_t operator()(const SamplerDesc& desc) const {

				size_t h = std::hash<uint8_t>{}((uint8_t)desc.Filter);
				HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.AddressU));
				HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.AddressV));
				HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.AddressW));
				HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.Reduction));
				HashCombine(h, std::hash<float>{}(desc.MipLODBias));
				HashCombine(h, std::hash<float>{}(desc.MinLOD));
				HashCombine(h, std::hash<float>{}(desc.MaxLOD));
				HashCombine(h, std::hash<float>{}(desc.MaxAnisotropy));
				HashCombine(h, std::hash<bool>{}(desc.EnableAnisotropy));

				return h;
			}
		};
	};

	class RHISampler : public RHIResource {
	public:
		RHISampler(const SamplerDesc& desc) : m_Desc(desc), m_RHIData(nullptr), m_ShaderIndex(UINT32_MAX) {}
		virtual ~RHISampler() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		const SamplerDesc& GetDesc() const { return m_Desc; }
		RHIData* GetRHIData() { return m_RHIData; }

		uint32_t GetShaderIndex() const { return m_ShaderIndex; }

	private:

		SamplerDesc m_Desc;
		RHIData* m_RHIData;

		uint32_t m_ShaderIndex;
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