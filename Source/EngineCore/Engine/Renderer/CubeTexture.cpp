#include <Engine/Renderer/CubeTexture.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>
#include <Generated/CubeMapFiltering.h>
#include <Generated/CubeMapGen.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Application.h>

#include <stb_image.h>

namespace Spike {

	RHICubeTexture::RHICubeTexture(const CubeTextureDesc& desc) : m_Desc(desc) {
		m_RHIData = 0;

		TextureViewDesc viewDesc{};
		viewDesc.BaseMip = 0;
		viewDesc.NumMips = desc.NumMips;
		viewDesc.BaseArrayLayer = 0;
		viewDesc.NumArrayLayers = 6;
		viewDesc.SourceTexture = this;

		m_TextureView = new RHITextureView(viewDesc);
	}

	void RHICubeTexture::InitRHI() {
		m_RHIData = GRHIDevice->CreateCubeTextureRHI(m_Desc);

		if (EnumHasAllFlags(m_Desc.UsageFlags, ETextureUsageFlags::ESampled) && m_Desc.AutoCreateSampler && !m_Desc.Sampler) {
			m_Desc.Sampler = GSamplerCache->Get(m_Desc.SamplerDesc);
		}

		m_TextureView->InitRHI();
	}

	void RHICubeTexture::ReleaseRHIImmediate() {

		m_TextureView->ReleaseRHIImmediate();
		delete m_TextureView;

		GRHIDevice->DestroyCubeTextureRHI(m_RHIData);
	}

	void RHICubeTexture::ReleaseRHI() {

		m_TextureView->ReleaseRHI();
		delete m_TextureView;

		GFrameRenderer->SubmitToFrameQueue([data = m_RHIData]() {
			GRHIDevice->DestroyCubeTextureRHI(data);
			});
	}


	CubeTexture::CubeTexture(const CubeTextureDesc& desc, UUID id) {
		m_ID = id;
		CreateResource(desc);
	}

	CubeTexture::~CubeTexture() {
		ReleaseResource();
	}

	Ref<CubeTexture> CubeTexture::Create(BinaryReadStream& stream, UUID id) {

		char magic[4] = {};
		stream >> magic;

		if (memcmp(magic, CUBE_TEXTURE_MAGIC, sizeof(char) * 4) != 0) {
			ENGINE_ERROR("Corrupted cube texture asset file: {}", (uint64_t)id);
			return nullptr;
		}

		CubeTextureHeader header{};
		std::vector<size_t> mipSizes{};
		stream >> header >> mipSizes;

		CubeTextureDesc desc{};
		desc.Size = header.Size;
		desc.Format = header.Format;
		desc.NumMips = (uint32_t)mipSizes.size();
		desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
		desc.SamplerDesc.Filter = header.Filter;
		desc.SamplerDesc.AddressU = header.AddressU;
		desc.SamplerDesc.AddressV = header.AddressV;
		desc.SamplerDesc.AddressW = header.AddressW;
		desc.SamplerDesc.AddressW = ESamplerAddress::EClamp;

		uint8_t* buff = new uint8_t[header.ByteSize];
		stream.ReadRaw(buff, header.ByteSize);

		Ref<CubeTexture> tex = CreateRef<CubeTexture>(desc, id);
		SUBMIT_RENDER_COMMAND(([rhi = tex->GetResource(), sizes = std::move(mipSizes), copySize = header.ByteSize, buff]() {

			size_t offset = 0;
			std::vector<RHIDevice::SubResourceCopyRegion> regions{};
			for (uint32_t m = 0; m < rhi->GetNumMips(); m++) {
				for (int f = 0; f < 6; f++) {

					RHIDevice::SubResourceCopyRegion& region = regions.emplace_back(RHIDevice::SubResourceCopyRegion{});
					region.ArrayLayer = f;
					region.DataOffset = offset;
					region.MipLevel = m;
					offset += sizes[m];
				}
			}
			GRHIDevice->CopyDataToTexture(buff, 0, rhi, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, regions, copySize);
			delete[] buff;
			}));

		return tex;
	}

	void CubeTexture::CreateResource(const CubeTextureDesc& desc) {

		m_RHIResource = new RHICubeTexture(desc);
		SafeRHIResourceInit(m_RHIResource);
	}

	void CubeTexture::ReleaseResource() {

		SafeRHIResourceRelease(m_RHIResource);
		m_RHIResource = nullptr;
	}
}