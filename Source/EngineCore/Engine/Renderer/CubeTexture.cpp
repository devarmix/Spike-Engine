#include <Engine/Renderer/CubeTexture.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>
#include <Generated/CubeMapFiltering.h>
#include <Generated/CubeMapGen.h>
#include <Engine/Renderer/FrameRenderer.h>

#include <stb_image.h>

namespace Spike {

	RHICubeTexture::RHICubeTexture(const CubeTextureDesc& desc) : m_Desc(desc) {

		m_RHIData = nullptr;

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

		// filter the texture
		{
			ShaderDesc shaderDesc{};
			shaderDesc.Type = EShaderType::ECompute;

			bool isNoneFilter = (m_Desc.FilterMode == ECubeTextureFilterMode::ENone);
			shaderDesc.Name = isNoneFilter ? "CubeMapGen" : "CubeMapFiltering";

			RHIShader* shader = GShaderManager->GetShaderFromCache(shaderDesc);

			Texture2DDesc offscreenDesc{};
			offscreenDesc.Width = m_Desc.Size;
			offscreenDesc.Height = m_Desc.Size;
			offscreenDesc.Format = m_Desc.Format;
			offscreenDesc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc;
			offscreenDesc.NumMips = 1;

			RHITexture2D* offscreen = new RHITexture2D(offscreenDesc);
			offscreen->InitRHI();

			GRHIDevice->ImmediateSubmit([&, this](RHICommandBuffer* cmd) {

				GRHIDevice->BarrierCubeTexture(cmd, this, EGPUAccessFlags::ENone, EGPUAccessFlags::ECopyDst);
				GRHIDevice->BarrierTexture2D(cmd, offscreen, EGPUAccessFlags::ENone, EGPUAccessFlags::EUAVCompute);

				RHIBindingSet* cubeSet = new RHIBindingSet(shader->GetLayouts()[0]);
				cubeSet->InitRHI();
				{
					cubeSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureUAV, offscreen->GetTextureView(), EGPUAccessFlags::EUAVCompute);
					cubeSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, m_Desc.SamplerTexture->GetTextureView(), EGPUAccessFlags::ESRV);
					cubeSet->AddSamplerWrite(2, 0, EShaderResourceType::ESampler, m_Desc.SamplerTexture->GetSampler());
				}

				for (int f = 0; f < 6; f++) {

					for (uint32_t m = 0; m < m_Desc.NumMips; m++) {

						uint32_t mipSize = m_Desc.Size >> m;
						if (mipSize < 1) mipSize = 1;

						if (isNoneFilter) {

							CubeMapGenPushData pushData{};
							pushData.FaceIndex = f;
							pushData.OutSize = mipSize;

							GRHIDevice->BindShader(cmd, shader, {cubeSet}, &pushData);
						}
						else {

							CubeMapFilteringPushData pushData{};
							pushData.FaceIndex = f;
							pushData.FilterType = (uint8_t)m_Desc.FilterMode;
							pushData.OutSize = mipSize;

							if (m_Desc.FilterMode == ECubeTextureFilterMode::ERadiance) {
								pushData.Roughness = (float)m / (float)(m_Desc.NumMips - 1);
							}

							GRHIDevice->BindShader(cmd, shader, { cubeSet }, &pushData);
						}

						uint32_t groupCount = GetComputeGroupCount(mipSize, 8);
						GRHIDevice->DispatchCompute(cmd, groupCount, groupCount, 1);

						// copy offscreen to our cube map
						{
							GRHIDevice->BarrierTexture2D(cmd, offscreen, EGPUAccessFlags::EUAVCompute, EGPUAccessFlags::ECopySrc);

							RHIDevice::TextureCopyRegion srcRegion{};
							srcRegion.BaseArrayLayer = 0;
							srcRegion.LayerCount = 1;
							srcRegion.MipLevel = 0;
							srcRegion.Offset = { 0, 0, 0 };

							RHIDevice::TextureCopyRegion dstRegion{};
							dstRegion.BaseArrayLayer = f;
							dstRegion.LayerCount = 1;
							dstRegion.MipLevel = m;
							dstRegion.Offset = { 0, 0, 0 };

							GRHIDevice->CopyTexture(cmd, offscreen, srcRegion, this, dstRegion, { mipSize, mipSize });
							GRHIDevice->BarrierTexture2D(cmd, offscreen, EGPUAccessFlags::ECopySrc, EGPUAccessFlags::EUAVCompute);
						}
					}
				}

				GRHIDevice->BarrierCubeTexture(cmd, this, EGPUAccessFlags::ECopyDst, EGPUAccessFlags::ESRV);
				});

			offscreen->ReleaseRHIImmediate();
			delete offscreen;
		}
	}

	void RHICubeTexture::ReleaseRHIImmediate() {

		GRHIDevice->DestroyCubeTextureRHI(m_RHIData);

		m_TextureView->ReleaseRHIImmediate();
		delete m_TextureView;
	}

	void RHICubeTexture::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([data = m_RHIData]() {
			GRHIDevice->DestroyCubeTextureRHI(data);
			});

		m_TextureView->ReleaseRHI();
		delete m_TextureView;
	}


	CubeTexture::CubeTexture(const CubeTextureDesc& desc) {

		CreateResource(desc);
	}

	CubeTexture::~CubeTexture() {

		ReleaseResource();
		ASSET_CORE_DESTROY();
	}

	Ref<CubeTexture> CubeTexture::Create(const CubeTextureDesc& desc) {

		return CreateRef<CubeTexture>(desc);
	}

	Ref<CubeTexture> CubeTexture::Create(const char* filePath, uint32_t size, const SamplerDesc& samplerDesc, ETextureFormat format) {

		SamplerDesc texSamplerDesc{};
		texSamplerDesc.Filter = ESamplerFilter::EBilinear;
		texSamplerDesc.AddressU = ESamplerAddress::EClamp;
		texSamplerDesc.AddressV = ESamplerAddress::EClamp;
		texSamplerDesc.AddressW = ESamplerAddress::EClamp;

		Ref<Texture2D> samplerTexture = Texture2D::Create(filePath,texSamplerDesc,  format);

		CubeTextureDesc desc{};
		desc.Size = size;
		desc.Format = format;
		desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
		desc.NumMips = 1;
		desc.SamplerDesc = samplerDesc;
		desc.FilterMode = ECubeTextureFilterMode::ENone;
		desc.SamplerTexture = samplerTexture->GetResource();

		return CubeTexture::Create(desc);
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