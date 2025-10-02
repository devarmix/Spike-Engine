#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Application.h>
#include <Engine/Core/Log.h>

namespace Spike {

	RHITexture2D::RHITexture2D(const Texture2DDesc& desc) : m_Desc(desc) {

		m_RHIData = 0;

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

		if (EnumHasAllFlags(m_Desc.UsageFlags, ETextureUsageFlags::ESampled) && m_Desc.AutoCreateSampler && !m_Desc.Sampler) {
			m_Desc.Sampler = GSamplerCache->Get(m_Desc.SamplerDesc);
		}

		m_TextureView->InitRHI();
	}

	void RHITexture2D::ReleaseRHIImmediate() {

		m_TextureView->ReleaseRHIImmediate();
		delete m_TextureView;

		GRHIDevice->DestroyTexture2DRHI(m_RHIData);
	}

	void RHITexture2D::ReleaseRHI() {

		m_TextureView->ReleaseRHI();
		delete m_TextureView;

		GFrameRenderer->SubmitToFrameQueue([data = m_RHIData]() {
			GRHIDevice->DestroyTexture2DRHI(data);
			});
	}

	Texture2D::Texture2D(const Texture2DDesc& desc, UUID id) {
		m_ID = id;
		CreateResource(desc);
	}

	Texture2D::~Texture2D() {
		ReleaseResource();
	}

	void Texture2D::CreateResource(const Texture2DDesc& desc) {

		m_RHIResource = new RHITexture2D(desc);
		SafeRHIResourceInit(m_RHIResource);
	}

	void Texture2D::ReleaseResource() {

		SafeRHIResourceRelease(m_RHIResource);
		m_RHIResource = nullptr;
	}

	Ref<Texture2D> Texture2D::Create(BinaryReadStream& stream, UUID id) {

		char magic[4] = {};
		stream >> magic;

		if (memcmp(magic, TEXTURE_2D_MAGIC, sizeof(char) * 4) != 0) {
			ENGINE_ERROR("Corrupted texture 2D asset file: {}", (uint64_t)id);
			return nullptr;
		}

		Texture2DHeader header{};
		std::vector<size_t> mipSizes{};
		stream >> header >> mipSizes;

		Texture2DDesc desc{};
		desc.Width = header.Width;
		desc.Height = header.Height;
		desc.Format = header.Format;
		desc.NumMips = (uint32_t)mipSizes.size();
		desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
		desc.SamplerDesc.Filter = header.Filter;
		desc.SamplerDesc.AddressU = header.AddressU;
		desc.SamplerDesc.AddressV = header.AddressV;
		desc.SamplerDesc.AddressW = ESamplerAddress::EClamp;

		uint8_t* buff = new uint8_t[header.ByteSize];
		stream.ReadRaw(buff, header.ByteSize);

		Ref<Texture2D> tex = CreateRef<Texture2D>(desc, id);
		SUBMIT_RENDER_COMMAND(([rhi = tex->GetResource(), sizes = std::move(mipSizes), copySize = header.ByteSize, buff]() {

			size_t offset = 0;
			std::vector<RHIDevice::SubResourceCopyRegion> regions{};
			for (uint32_t m = 0; m < rhi->GetNumMips(); m++) {

				RHIDevice::SubResourceCopyRegion& region = regions.emplace_back(RHIDevice::SubResourceCopyRegion{});
				region.ArrayLayer = 0;
				region.DataOffset = offset;
				region.MipLevel = m;
				offset += sizes[m];
			}
			GRHIDevice->CopyDataToTexture(buff, 0, rhi, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, regions, copySize);
			delete[] buff;
			}));

		return tex;
	}
}