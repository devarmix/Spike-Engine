#include <Engine/Renderer/DefaultFeatures.h>
#include <Engine/Core/Log.h>
#include <Engine/Utils/RenderUtils.h>

#include <Generated/IndirectCull.h>
#include <Generated/DepthPyramid.h>
#include <Generated/DeferredLighting.h>
#include <Generated/SSAOGen.h>
#include <Generated/SSAOComposite.h>
#include <Generated/BloomDownSample.h>
#include <Generated/BloomUpSample.h>
#include <Generated/ToneMap.h>
#include <Generated/Skybox.h>
#include <Generated/SMAA_Edge.h>
#include <Generated/SMAA_Weights.h>
#include <Generated/SMAA_Neighbors.h>
#include <Generated/FXAA.h>

#include <random>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace Spike {

	GBufferFeature::GBufferFeature() {

		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "IndirectCull";

			m_CullShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "DepthPyramid";

			m_HzbShader = GShaderManager->GetShaderFromCache(desc);
		}
	}

	GBufferFeature::~GBufferFeature() {}

	void GBufferFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		SamplerDesc samplerDesc{};
		samplerDesc.Filter = ESamplerFilter::EBilinear;
		samplerDesc.AddressU = ESamplerAddress::EClamp;
		samplerDesc.AddressV = ESamplerAddress::EClamp;
		samplerDesc.AddressW = ESamplerAddress::EClamp;

		Texture2DDesc texDesc{};
		texDesc.Width = context.OutTexture->GetSizeXYZ().x;
		texDesc.Height = context.OutTexture->GetSizeXYZ().y;
		texDesc.NumMips = 1;
		texDesc.UsageFlags = ETextureUsageFlags::EColorTarget | ETextureUsageFlags::ESampled;
		texDesc.Format = ETextureFormat::ERGBA16F;
		texDesc.SamplerDesc = samplerDesc;

		RDGHandle albedoTex = graphBuilder->CreateRDGTexture2D("GBuffer-Albedo", texDesc);
		RDGHandle normalTex = graphBuilder->CreateRDGTexture2D("GBuffer-Normal", texDesc);

		texDesc.Format = ETextureFormat::ERGBA8U;
		RDGHandle materialTex = graphBuilder->CreateRDGTexture2D("GBuffer-Material", texDesc);

		texDesc.Format = ETextureFormat::ED32F;
		texDesc.UsageFlags = ETextureUsageFlags::EDepthTarget | ETextureUsageFlags::ESampled;
		RDGHandle depthTex = graphBuilder->CreateRDGTexture2D("GBuffer-Depth", texDesc);

		BufferDesc uboDesc{};
		uboDesc.Size = sizeof(WorldGPUData);
		uboDesc.UsageFlags = EBufferUsageFlags::EConstant;
		uboDesc.MemUsage = EBufferMemUsage::ECPUToGPU;
		RDGHandle sceneUBO = graphBuilder->CreateRDGBuffer("Scene-UBO", uboDesc);

		BufferDesc batchSSBODesc{};
		batchSSBODesc.Size = sizeof(uint32_t) * MAX_SHADERS_PER_WORLD;
		batchSSBODesc.UsageFlags = EBufferUsageFlags::EStorage;
		batchSSBODesc.MemUsage = EBufferMemUsage::ECPUToGPU;
		RDGHandle batchSSBO = graphBuilder->CreateRDGBuffer("Batch-SSBO", batchSSBODesc);

		auto roundUpToPowerOfTwo = [](float value) {

			float valueBase = glm::log2(value);
			if (glm::fract(valueBase) == 0.0f) {
				return (uint32_t)value;
			}
			return (uint32_t)glm::pow(2.0f, glm::trunc(valueBase + 1.0f));
			};

		SamplerDesc hzbSamplerDesc{};
		hzbSamplerDesc.Filter = ESamplerFilter::EBilinear;
		hzbSamplerDesc.AddressU = ESamplerAddress::EClamp; 
		hzbSamplerDesc.AddressV = ESamplerAddress::EClamp;
		hzbSamplerDesc.AddressW = ESamplerAddress::EClamp;
		hzbSamplerDesc.MinLOD = 0.0f;
		hzbSamplerDesc.MaxLOD = 16.0f;
		hzbSamplerDesc.Reduction = ESamplerReduction::EMinimum;

		uint32_t hzbSize = roundUpToPowerOfTwo((float)std::max(context.OutTexture->GetSizeXYZ().x, context.OutTexture->GetSizeXYZ().y));

		Texture2DDesc hzbDesc{};
		hzbDesc.Width = hzbSize;
		hzbDesc.Height = hzbSize;
		hzbDesc.Format = ETextureFormat::ER32F;
		hzbDesc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopyDst;
		hzbDesc.NumMips = GetNumTextureMips(hzbSize, hzbSize);
		hzbDesc.SamplerDesc = hzbSamplerDesc;

		RDGHandle hzbTex = graphBuilder->CreateRDGTexture2D("GBuffer-Hzb", hzbDesc);

		graphBuilder->RegisterExternalBuffer(proxy->ObjectsBuffer);
		graphBuilder->RegisterExternalBuffer(proxy->DrawCommandsBuffer);
		graphBuilder->RegisterExternalBuffer(proxy->DrawCountsBuffer);
		//graphBuilder->RegisterExternalBuffer(proxy->BatchOffsetsBuffer);
		graphBuilder->RegisterExternalBuffer(proxy->VisibilityBuffer);

		graphBuilder->AddPass(
			{ albedoTex, normalTex, materialTex, depthTex, hzbTex },
			{ sceneUBO, batchSSBO }, 
			ERendererStage::EOpaqueRender, 
			[=, this](RHICommandBuffer* cmd) {

				RHIBuffer* ubo = graphBuilder->GetBufferResource(sceneUBO);
				{
					WorldGPUData& worldData = *(WorldGPUData*)ubo->GetMappedData();
					worldData.View = cameraData->View;
					worldData.Proj = cameraData->Proj;
					worldData.ViewProj = cameraData->Proj * cameraData->View;
					worldData.InverseProj = glm::inverse(cameraData->Proj);
					worldData.InverseView = glm::inverse(cameraData->View);
					worldData.CameraPos = Vec4(cameraData->Position, 1.f);

					glm::mat4& vp = worldData.ViewProj;
					worldData.FrustumPlanes[0] = glm::vec4(
						vp[0][3] + vp[0][0], vp[1][3] + vp[1][0],
						vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
					worldData.FrustumPlanes[1] = glm::vec4(
						vp[0][3] - vp[0][0], vp[1][3] - vp[1][0],
						vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
					worldData.FrustumPlanes[2] = glm::vec4(
						vp[0][3] + vp[0][1], vp[1][3] + vp[1][1],
						vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
					worldData.FrustumPlanes[3] = glm::vec4(
						vp[0][3] - vp[0][1], vp[1][3] - vp[1][1],
						vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
					worldData.FrustumPlanes[4] = glm::vec4(
						vp[0][3] + vp[0][2], vp[1][3] + vp[1][2],
						vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
					worldData.FrustumPlanes[5] = glm::vec4(
						vp[0][3] - vp[0][2], vp[1][3] - vp[1][2],
						vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

					for (int i = 0; i < 6; i++) {
						float length = glm::length(glm::vec3(worldData.FrustumPlanes[i]));
						worldData.FrustumPlanes[i] /= length;
					}

					worldData.P00 = worldData.Proj[0][0];
					worldData.P11 = worldData.Proj[1][1];
					worldData.NearProj = cameraData->NearProj;
					worldData.FarProj = cameraData->FarProj;
					worldData.LightsCount = proxy->LightsVB.Size();
					worldData.ObjectsCount = proxy->ObjectsVB.Size();

					// TODO: Make serializable and changeble in world settings
					worldData.SunColor = { 1.0f, 0.7f, 0.6f, 1.0f };
					worldData.SunIntensity = 15;

					// should probably be normalized on cpu
					worldData.SunDirection = Vec4(-58.823f, -588.235f, 735.394f, 0.0f);
				}

				std::vector<uint32_t> batchOffsets;
				RHIBuffer* bSSBO = graphBuilder->GetBufferResource(batchSSBO);
				batchOffsets.resize(proxy->Batches.size());
				{
					uint32_t* offsetsBuffer = (uint32_t*)bSSBO->GetMappedData();
					for (int i = 0; i < proxy->Batches.size(); i++) {

						if (i > 0) {
							batchOffsets[i] = proxy->Batches[i - 1].CommandsCount + batchOffsets[i - 1];
						}
						else {
							batchOffsets[i] = 0;
						}

						offsetsBuffer[i] = batchOffsets[i];
					}
				}

				std::vector<RHITextureView*> hzbViews;
				RHITexture2D* hzb = graphBuilder->GetTextureResource(hzbTex);

				for (uint32_t i = 0; i < hzb->GetNumMips(); i++) {

					TextureViewDesc viewDesc{};
					viewDesc.BaseMip = i;
					viewDesc.NumMips = 1;
					viewDesc.BaseArrayLayer = 0;
					viewDesc.NumArrayLayers = 1;
					viewDesc.SourceTexture = hzb;

					RHITextureView* view = GRDGPool->GetOrCreateTextureView(viewDesc);
					hzbViews.push_back(view);
				}

				RHITexture2D* albedo = graphBuilder->GetTextureResource(albedoTex);
				RHITexture2D* normal = graphBuilder->GetTextureResource(normalTex);
				RHITexture2D* material = graphBuilder->GetTextureResource(materialTex);
				RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);

				RHIBindingSet* cullSet = GRDGPool->GetOrCreateBindingSet(m_CullShader->GetLayouts()[0]);
				{
					cullSet->AddBufferWrite(0, 0, EShaderResourceType::EBufferUAV, proxy->DrawCommandsBuffer,
						proxy->DrawCommandsBuffer->GetSize(), 0);
					cullSet->AddBufferWrite(1, 0, EShaderResourceType::EBufferUAV, proxy->DrawCountsBuffer,
						proxy->DrawCountsBuffer->GetSize(), 0);
					cullSet->AddBufferWrite(2, 0, EShaderResourceType::EBufferSRV, bSSBO,
						bSSBO->GetSize(), 0);
					cullSet->AddBufferWrite(3, 0, EShaderResourceType::EBufferSRV, proxy->VisibilityBuffer,
						proxy->VisibilityBuffer->GetSize(), 0);
					//cullSet->AddBufferWrite(4, 0, EShaderResourceType::EBufferUAV, frameData.VisibilityBuffer,
					//	sizeof(uint32_t) * scene->Objects.size(), sizeof(uint32_t) * frameData.ObjectsOffset);
					cullSet->AddTextureWrite(4, 0, EShaderResourceType::ETextureSRV, hzb->GetTextureView(), EGPUAccessFlags::ESRVCompute);
					cullSet->AddSamplerWrite(5, 0, EShaderResourceType::ESampler, hzb->GetSampler());
					cullSet->AddBufferWrite(6, 0, EShaderResourceType::EBufferSRV, proxy->ObjectsBuffer,
						proxy->ObjectsBuffer->GetSize(), 0);
					cullSet->AddBufferWrite(7, 0, EShaderResourceType::EConstantBuffer, ubo, ubo->GetSize(), 0);
				}

				auto cullGeometry = [=, this](bool prepass) {

					IndirectCullPushData pushData{};
					pushData.IsPrepass = prepass ? 1 : 0; 

					if (!prepass) {

						pushData.PyramidSize = hzbSize;
						pushData.CullZNear = cameraData->Proj[3][2]; 
					}

					GRHIDevice->BindShader(cmd, m_CullShader, {cullSet}, &pushData);

					uint32_t groupSize = uint32_t((proxy->ObjectsVB.Size() / 256) + 1);
					GRHIDevice->DispatchCompute(cmd, groupSize, 1, 1);

					graphBuilder->BarrierRDGBuffer(cmd, proxy->DrawCommandsBuffer, 
						proxy->DrawCommandsBuffer->GetSize(), 0, EGPUAccessFlags::EIndirectArgs);
					graphBuilder->BarrierRDGBuffer(cmd, proxy->DrawCountsBuffer, 
						proxy->DrawCountsBuffer->GetSize(), 0, EGPUAccessFlags::EIndirectArgs);
					};

				RHIBindingSet* meshDrawSet = GRDGPool->GetOrCreateBindingSet(GShaderManager->GetMeshDrawLayout());
				{
					meshDrawSet->AddBufferWrite(0, 0, EShaderResourceType::EConstantBuffer, ubo, sizeof(WorldGPUData), 0);
					meshDrawSet->AddBufferWrite(1, 0, EShaderResourceType::EBufferSRV, proxy->ObjectsBuffer,
						proxy->ObjectsBuffer->GetSize(), 0);
				}

				auto drawGeometry = [=](bool prepass) {

					Vec4 colorClear = { 0.0f, 0.0f, 0.0f, 1.0f };
					Vec2 depthClear = { 0.0f, 0.0f };

					RHIDevice::RenderInfo info{};
					info.ColorTargets = { albedo->GetTextureView(), normal->GetTextureView(), material->GetTextureView() };
					info.DepthTarget = depth->GetTextureView();
					info.ColorClear = prepass ? &colorClear : nullptr;
					info.DepthClear = prepass ? &depthClear : nullptr;
					info.DrawSize = { context.OutTexture->GetSizeXYZ().x, context.OutTexture->GetSizeXYZ().y };

					GRHIDevice->BeginRendering(cmd, info);

					for (int i = 0; i < proxy->Batches.size(); i++) {

						WorldDrawBatch currBatch = proxy->Batches[i];
						GRHIDevice->BindShader(cmd, currBatch.Shader, {meshDrawSet, GShaderManager->GetMaterialSet()});

						uint32_t stride = sizeof(DrawIndirectCommand);
						uint64_t commandOffset = batchOffsets[i];
						uint64_t countOffset = i;

						GRHIDevice->DrawIndirectCount(cmd, proxy->DrawCommandsBuffer, stride * commandOffset, proxy->DrawCountsBuffer,
							sizeof(uint32_t) * countOffset, proxy->ObjectsVB.Size(), stride);
					}

					GRHIDevice->EndRendering(cmd); 
					};

				// first cull pass
				{
					graphBuilder->BarrierRDGTexture2D(cmd, hzb, EGPUAccessFlags::ESRVCompute);

					cullGeometry(true);

					graphBuilder->BarrierRDGTexture2D(cmd, albedo, EGPUAccessFlags::EColorTarget);
					graphBuilder->BarrierRDGTexture2D(cmd, normal, EGPUAccessFlags::EColorTarget);
					graphBuilder->BarrierRDGTexture2D(cmd, material, EGPUAccessFlags::EColorTarget);
					graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::EDepthTarget);

					drawGeometry(true);
				}

				// build hzb
				{
					graphBuilder->BarrierRDGTexture2D(cmd, hzb, EGPUAccessFlags::EUAVCompute);
					graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::ESRVCompute);

					for (uint32_t i = 0; i < hzb->GetNumMips(); i++) {

						RHIBindingSet* hzbSet = GRDGPool->GetOrCreateBindingSet(m_HzbShader->GetLayouts()[0]);

						hzbSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureUAV, hzbViews[i], EGPUAccessFlags::EUAVCompute);
						if (i == 0) {
							hzbSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, depth->GetTextureView(), EGPUAccessFlags::ESRVCompute);
						}
						else {
							hzbSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, hzbViews[i - 1], EGPUAccessFlags::EUAVCompute);
						}
						hzbSet->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, hzb->GetSampler());

						DepthPyramidPushData pushData{};
						uint32_t levelSize = hzbSize >> i;
						pushData.MipSize = levelSize;

						GRHIDevice->BindShader(cmd, m_HzbShader, {hzbSet}, &pushData);

						uint32_t groupCount = GetComputeGroupCount(levelSize, 32);
						GRHIDevice->DispatchCompute(cmd, groupCount, groupCount, 1);

						graphBuilder->BarrierRDGTexture2D(cmd, hzb, EGPUAccessFlags::EUAVCompute);
					}

					graphBuilder->BarrierRDGTexture2D(cmd, hzb, EGPUAccessFlags::ESRV);
				}

				// second cull pass
				{
					graphBuilder->FillRDGBuffer(cmd, proxy->DrawCountsBuffer, 
						proxy->DrawCountsBuffer->GetSize(), 0, 0, EGPUAccessFlags::EUAVCompute);
					graphBuilder->BarrierRDGBuffer(cmd, proxy->DrawCommandsBuffer, 
						proxy->DrawCommandsBuffer->GetSize(), 0, EGPUAccessFlags::EUAVCompute);

					cullGeometry(false);
					graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::EDepthTarget);
					drawGeometry(false);
				}

				graphBuilder->BarrierRDGTexture2D(cmd, albedo, EGPUAccessFlags::ESRV);
				graphBuilder->BarrierRDGTexture2D(cmd, normal, EGPUAccessFlags::ESRV);
				graphBuilder->BarrierRDGTexture2D(cmd, material, EGPUAccessFlags::ESRV);
				graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::ESRV);
			});
	}


	DeferredLightingFeature::DeferredLightingFeature() {

		ShaderDesc desc{};
		desc.Type = EShaderType::EGraphics;
		desc.Name = "DeferredLighting";
		desc.ColorTargetFormats = { ETextureFormat::ERGBA16F };

		m_LightingShader = GShaderManager->GetShaderFromCache(desc);
	}

	DeferredLightingFeature::~DeferredLightingFeature() {}

	void DeferredLightingFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		RDGHandle albedoTex = graphBuilder->FindRDGTexture2D("GBuffer-Albedo");
		RDGHandle normalTex = graphBuilder->FindRDGTexture2D("GBuffer-Normal");
		RDGHandle materialTex = graphBuilder->FindRDGTexture2D("GBuffer-Material");
		RDGHandle depthTex = graphBuilder->FindRDGTexture2D("GBuffer-Depth");
		RDGHandle sceneUBO = graphBuilder->FindRDGBuffer("Scene-UBO");

		graphBuilder->RegisterExternalTexture2D(context.OutTexture);
		graphBuilder->RegisterExternalBuffer(proxy->LightsBuffer);

		graphBuilder->AddPass(
			{albedoTex, normalTex, materialTex, depthTex},
			{sceneUBO},
			ERendererStage::ELightsRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* albedo = graphBuilder->GetTextureResource(albedoTex);
				RHITexture2D* normal = graphBuilder->GetTextureResource(normalTex);
				RHITexture2D* material = graphBuilder->GetTextureResource(materialTex);
				RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);
				RHITexture2D* brdf = GFrameRenderer->GetBRDFLut();
				RHIBuffer* ubo = graphBuilder->GetBufferResource(sceneUBO);

				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::EColorTarget);

				RHIBindingSet* lightingSet = GRDGPool->GetOrCreateBindingSet(m_LightingShader->GetLayouts()[0]);
				{
					lightingSet->AddBufferWrite(0, 0, EShaderResourceType::EConstantBuffer, ubo, ubo->GetSize(), 0);
					lightingSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, albedo->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureSRV, normal->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureSRV, material->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddTextureWrite(4, 0, EShaderResourceType::ETextureSRV, depth->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddTextureWrite(5, 0, EShaderResourceType::ETextureSRV, context.EnvironmentTexture->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddTextureWrite(6, 0, EShaderResourceType::ETextureSRV, context.IrradianceTexture->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddTextureWrite(7, 0, EShaderResourceType::ETextureSRV, brdf->GetTextureView(), EGPUAccessFlags::ESRV);
					lightingSet->AddSamplerWrite(8, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
					lightingSet->AddSamplerWrite(9, 0, EShaderResourceType::ESampler, context.EnvironmentTexture->GetSampler());
					lightingSet->AddBufferWrite(10, 0, EShaderResourceType::EBufferSRV, proxy->LightsBuffer, proxy->LightsBuffer->GetSize(), 0);
				}

				DeferredLightingPushData pushData{};
				pushData.EnvMapNumMips = context.EnvironmentTexture->GetNumMips();

				Vec4 colorClear = { 0.0f, 0.0f, 0.0f, 1.0f };

				RHIDevice::RenderInfo info{};
				info.ColorTargets = { context.OutTexture->GetTextureView() };
				info.DepthTarget = nullptr;
				info.ColorClear = &colorClear;
				info.DepthClear = nullptr;
				info.DrawSize = { context.OutTexture->GetSizeXYZ().x, context.OutTexture->GetSizeXYZ().y };

				GRHIDevice->BeginRendering(cmd, info);
				GRHIDevice->BindShader(cmd, m_LightingShader, {lightingSet}, &pushData);
				GRHIDevice->Draw(cmd, 3, 1, 0, 0);
				GRHIDevice->EndRendering(cmd);

				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
			});
	}


	SkyboxFeature::SkyboxFeature() {

		ShaderDesc desc{};
		desc.Type = EShaderType::EGraphics;
		desc.Name = "Skybox";
		desc.ColorTargetFormats = { ETextureFormat::ERGBA16F };
		desc.EnableDepthTest = true;

		m_SkyboxShader = GShaderManager->GetShaderFromCache(desc);
	}

	SkyboxFeature::~SkyboxFeature() {}

	void SkyboxFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		RDGHandle depthTex = graphBuilder->FindRDGTexture2D("GBuffer-Depth");
		RDGHandle sceneUBO = graphBuilder->FindRDGBuffer("Scene-UBO");
		
		graphBuilder->AddPass(
			{ depthTex },
			{ sceneUBO },
			ERendererStage::EAfterLightsRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);
				RHIBuffer* ubo = graphBuilder->GetBufferResource(sceneUBO);

				graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::EDepthTarget);
				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::EColorTarget);

				RHIBindingSet* skyboxSet = GRDGPool->GetOrCreateBindingSet(m_SkyboxShader->GetLayouts()[0]);
				{
					skyboxSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, context.EnvironmentTexture->GetTextureView(), EGPUAccessFlags::ESRV);
					skyboxSet->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, context.EnvironmentTexture->GetSampler());
					skyboxSet->AddBufferWrite(2, 0, EShaderResourceType::EConstantBuffer, ubo, ubo->GetSize(), 0);
				}

				RHIDevice::RenderInfo info{};
				info.ColorTargets = { context.OutTexture->GetTextureView() };
				info.DepthTarget = depth->GetTextureView();
				info.ColorClear = nullptr;
				info.DepthClear = nullptr;
				info.DrawSize = { context.OutTexture->GetSizeXYZ().x, context.OutTexture->GetSizeXYZ().y };

				SkyboxPushData pushData{};
				pushData.Intensity = 2.5f;

				GRHIDevice->BindShader(cmd, m_SkyboxShader, {skyboxSet}, &pushData);
				GRHIDevice->BeginRendering(cmd, info);
				GRHIDevice->Draw(cmd, 3, 1, 0, 0);
				GRHIDevice->EndRendering(cmd);

				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
				graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::ESRV);
			});
	}


	SSAOFeature::SSAOFeature() {

		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "SSAOGen";

			m_GenShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "SSAOComposite";

			m_CompositeShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
			std::default_random_engine generator;

			Vec4* ssaoNoise = new Vec4[16];

			for (int i = 0; i < 16; i++)
			{
				glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
				ssaoNoise[i] = Vec4(noise, 1.0f);
			}

			SamplerDesc samplerDesc{};
			samplerDesc.Filter = ESamplerFilter::EPoint;
			samplerDesc.AddressU = ESamplerAddress::EClamp;
			samplerDesc.AddressV = ESamplerAddress::EClamp;
			samplerDesc.AddressW = ESamplerAddress::EClamp;

			Texture2DDesc desc{};
			desc.Width = 4;
			desc.Height = 4;
			desc.Format = ETextureFormat::ERGBA32F;
			desc.NumMips = 1;
			desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
			desc.SamplerDesc = samplerDesc;

			m_NoiseTexture = new RHITexture2D(desc);
			m_NoiseTexture->InitRHI();
			{
				RHIDevice::SubResourceCopyRegion region{ 0, 0, 0 };
				GRHIDevice->CopyDataToTexture(ssaoNoise, 0, m_NoiseTexture, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, { region }, sizeof(Vec4) * 16);
				delete[] ssaoNoise;
			}
		}
		{
			std::uniform_real_distribution<float> randomFloats(0.0, 1.0); 
			std::default_random_engine generator;
			std::vector<Vec4> ssaoKernel;

			for (uint32_t i = 0; i < 64; ++i)
			{
				Vec3 sample(
					randomFloats(generator) * 2.0 - 1.0,
					randomFloats(generator) * 2.0 - 1.0,
					randomFloats(generator));
				sample = glm::normalize(sample);
				sample *= randomFloats(generator);
				ssaoKernel.push_back(Vec4(sample, 1.0f));
			}

			BufferDesc desc{};
			desc.Size = sizeof(Vec3) * ssaoKernel.size();
			desc.UsageFlags = EBufferUsageFlags::EConstant;
			desc.MemUsage = EBufferMemUsage::ECPUToGPU;

			m_KernelBuffer = new RHIBuffer(desc);
			m_KernelBuffer->InitRHI();

			memcpy(m_KernelBuffer->GetMappedData(), ssaoKernel.data(), desc.Size);
		}
	}

	SSAOFeature::~SSAOFeature() {

		m_NoiseTexture->ReleaseRHI();
		delete m_NoiseTexture;

		m_KernelBuffer->ReleaseRHI();
		delete m_KernelBuffer;
	}

	void SSAOFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		RDGHandle normalTex = graphBuilder->FindRDGTexture2D("GBuffer-Normal");
		RDGHandle depthTex = graphBuilder->FindRDGTexture2D("GBuffer-Depth");

		SamplerDesc samplerDesc{};
		samplerDesc.Filter = ESamplerFilter::EBilinear;
		samplerDesc.AddressU = ESamplerAddress::EClamp;
		samplerDesc.AddressV = ESamplerAddress::EClamp;
		samplerDesc.AddressW = ESamplerAddress::EClamp;

		Texture2DDesc desc{};
		desc.Width = context.OutTexture->GetSizeXYZ().x;
		desc.Height = context.OutTexture->GetSizeXYZ().y;
		desc.Format = ETextureFormat::ER32F;
		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ESampled;
		desc.NumMips = 1;
		desc.SamplerDesc = samplerDesc;

		RDGHandle ssaoTex = graphBuilder->CreateRDGTexture2D("SSAO-Main", desc);

		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc;
		desc.Format = ETextureFormat::ERGBA16F;
		RDGHandle ssaoCompositeTex = graphBuilder->CreateRDGTexture2D("SSAO-Composite", desc);
		RDGHandle sceneUBO = graphBuilder->FindRDGBuffer("Scene-UBO");

		graphBuilder->AddPass(
			{ssaoTex, ssaoCompositeTex },
			{sceneUBO},
			ERendererStage::EPostProcessingRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* ssao = graphBuilder->GetTextureResource(ssaoTex);
				RHITexture2D* ssaoComposite = graphBuilder->GetTextureResource(ssaoCompositeTex);

				// gen ssao
				{
					graphBuilder->BarrierRDGTexture2D(cmd, ssao, EGPUAccessFlags::EUAVCompute);

					RHITexture2D* normal = graphBuilder->GetTextureResource(normalTex);
					RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);
					RHIBuffer* ubo = graphBuilder->GetBufferResource(sceneUBO);

					RHIBindingSet* genSet = GRDGPool->GetOrCreateBindingSet(m_GenShader->GetLayouts()[0]);
					{
						genSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, depth->GetTextureView(), EGPUAccessFlags::ESRV);
						genSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, normal->GetTextureView(), EGPUAccessFlags::ESRV);
						genSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureSRV, m_NoiseTexture->GetTextureView(), EGPUAccessFlags::ESRV);
						genSet->AddSamplerWrite(3, 0, EShaderResourceType::ESampler, m_NoiseTexture->GetSampler());
						genSet->AddSamplerWrite(4, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
						genSet->AddTextureWrite(5, 0, EShaderResourceType::ETextureUAV, ssao->GetTextureView(), EGPUAccessFlags::EUAVCompute);
						genSet->AddBufferWrite(6, 0, EShaderResourceType::EConstantBuffer, m_KernelBuffer, m_KernelBuffer->GetSize(), 0);
						genSet->AddBufferWrite(7, 0, EShaderResourceType::EConstantBuffer, ubo, ubo->GetSize(), 0);
					}

					SSAOGenPushData pushData{};
					pushData.Radius = 0.5f;
					pushData.Bias = 0.025f;
					pushData.NumSamples = 64;
					pushData.Intensity = 1.0f;
					pushData.TexSize = { ssao->GetSizeXYZ().x,  ssao->GetSizeXYZ().y };

					GRHIDevice->BindShader(cmd, m_GenShader, {genSet}, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(ssao->GetSizeXYZ().x, 32);
					uint32_t groupCountY = GetComputeGroupCount(ssao->GetSizeXYZ().y, 32);

					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
					graphBuilder->BarrierRDGTexture2D(cmd, ssao, EGPUAccessFlags::ESRVCompute);
				}

				// composite
				{
					graphBuilder->BarrierRDGTexture2D(cmd, ssaoComposite, EGPUAccessFlags::EUAVCompute);

					RHIBindingSet* compSet = GRDGPool->GetOrCreateBindingSet(m_CompositeShader->GetLayouts()[0]);
					{
						compSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, ssao->GetTextureView(), EGPUAccessFlags::ESRVCompute);
						compSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRV);
						compSet->AddSamplerWrite(2, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
						compSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureUAV, ssaoComposite->GetTextureView(), EGPUAccessFlags::EUAVCompute);
					}

					SSAOCompositePushData pushData{};
					pushData.TexSize = { ssaoComposite->GetSizeXYZ().x,  ssaoComposite->GetSizeXYZ().y };

					GRHIDevice->BindShader(cmd, m_CompositeShader, {compSet}, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(ssaoComposite->GetSizeXYZ().x, 32);
					uint32_t groupCountY = GetComputeGroupCount(ssaoComposite->GetSizeXYZ().y, 32);

					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
					graphBuilder->BarrierRDGTexture2D(cmd, ssaoComposite, EGPUAccessFlags::ECopySrc);
				}

				// copy ssao to main tex
				{
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ECopyDst);

					RHIDevice::TextureCopyRegion region{};
					region.BaseArrayLayer = 0;
					region.LayerCount = 1;
					region.MipLevel = 0;
					region.Offset = { 0.0f, 0.0f, 0.0f };

					GRHIDevice->CopyTexture(cmd, ssaoComposite, region, context.OutTexture, region, { context.OutTexture->GetSizeXYZ().x,  context.OutTexture->GetSizeXYZ().y });
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
				}
			});
	}


	BloomFeature::BloomFeature() {

		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "BloomDownSample";

			m_DownSampleShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "BloomUpSample";

			m_UpSampleShader = GShaderManager->GetShaderFromCache(desc);
		}
	}

	BloomFeature::~BloomFeature() {}

	void BloomFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		SamplerDesc samplerDesc{};
		samplerDesc.Filter = ESamplerFilter::EBilinear;
		samplerDesc.AddressU = ESamplerAddress::EClamp;
		samplerDesc.AddressV = ESamplerAddress::EClamp;
		samplerDesc.AddressW = ESamplerAddress::EClamp;

		uint32_t outWidth = context.OutTexture->GetSizeXYZ().x;
		uint32_t outHeight = context.OutTexture->GetSizeXYZ().y;

		Texture2DDesc desc{};
		desc.Width = outWidth >> 1;
		desc.Height = outHeight >> 1;
		desc.Format = ETextureFormat::ERGBA16F;
		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ESampled;

		uint32_t numMips = GetNumTextureMips(desc.Width, desc.Height);
		desc.NumMips = numMips;
		desc.SamplerDesc = samplerDesc;

		RDGHandle bloomDownTex = graphBuilder->CreateRDGTexture2D("Bloom-Downsample", desc);

		desc.NumMips = numMips - 1;
		RDGHandle bloomUpTex = graphBuilder->CreateRDGTexture2D("Bloom-Upsample", desc);

		desc.NumMips = 1;
		desc.Width = outWidth;
		desc.Height = outHeight;
		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc;

		RDGHandle bloomCompositeTex = graphBuilder->CreateRDGTexture2D("Bloom-Composite", desc);

		graphBuilder->AddPass(
			{bloomDownTex, bloomUpTex, bloomCompositeTex},
			{},
			ERendererStage::EPostProcessingRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* bloomDown = graphBuilder->GetTextureResource(bloomDownTex);
				std::vector<RHITextureView*> downViews;
				for (uint32_t i = 0; i < bloomDown->GetNumMips(); i++) {

					TextureViewDesc viewDesc{};
					viewDesc.BaseArrayLayer = 0;
					viewDesc.BaseMip = i;
					viewDesc.NumArrayLayers = 1;
					viewDesc.NumMips = 1;
					viewDesc.SourceTexture = bloomDown;

					RHITextureView* view = GRDGPool->GetOrCreateTextureView(viewDesc);
					downViews.push_back(view);
				}

				RHITexture2D* bloomUp = graphBuilder->GetTextureResource(bloomUpTex);
				std::vector<RHITextureView*> upViews;
				for (uint32_t i = 0; i < bloomUp->GetNumMips(); i++) {

					TextureViewDesc viewDesc{};
					viewDesc.BaseArrayLayer = 0;
					viewDesc.BaseMip = i;
					viewDesc.NumArrayLayers = 1;
					viewDesc.NumMips = 1;
					viewDesc.SourceTexture = bloomUp;

					RHITextureView* view = GRDGPool->GetOrCreateTextureView(viewDesc);
					upViews.push_back(view);
				}

				RHITexture2D* bloomComposite = graphBuilder->GetTextureResource(bloomCompositeTex);

				// downsample
				{
					graphBuilder->BarrierRDGTexture2D(cmd, bloomDown, EGPUAccessFlags::EUAVCompute);

					for (uint32_t i = 0; i < bloomDown->GetNumMips(); i++) {

						RHIBindingSet* downSet = GRDGPool->GetOrCreateBindingSet(m_DownSampleShader->GetLayouts()[0]);

						if (i == 0) {
							downSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRV);
						}
						else {
							downSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, downViews[i - 1], EGPUAccessFlags::EUAVCompute);
						}
						downSet->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
						downSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureUAV, downViews[i], EGPUAccessFlags::EUAVCompute);

						uint32_t srcWidth = outWidth >> i;
						uint32_t srcHeight = outHeight >> i;

						uint32_t levelWidth = srcWidth >> 1;
						uint32_t levelHeight = srcHeight >> 1;

						BloomDownSamplePushData pushData{};
						pushData.SrcSize = { srcWidth, srcHeight };
						pushData.OutSize = { levelWidth, levelHeight };
						pushData.MipLevel = i;
						pushData.Threadshold = 2.0f;
						pushData.SoftThreadshold = 0.5f;

						GRHIDevice->BindShader(cmd, m_DownSampleShader, {downSet}, &pushData);

						uint32_t groupCountX = GetComputeGroupCount(levelWidth, 32);
						uint32_t groupCountY = GetComputeGroupCount(levelHeight, 32);

						GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
						graphBuilder->BarrierRDGTexture2D(cmd, bloomDown, EGPUAccessFlags::EUAVCompute);
					}

					graphBuilder->BarrierRDGTexture2D(cmd, bloomDown, EGPUAccessFlags::ESRVCompute);
				}

				// upsample
				{
					graphBuilder->BarrierRDGTexture2D(cmd, bloomUp, EGPUAccessFlags::EUAVCompute);
					uint32_t mips = bloomUp->GetNumMips() - 1;

					for (int i = mips; i >= 0; i--) {

						RHIBindingSet* upSet = GRDGPool->GetOrCreateBindingSet(m_UpSampleShader->GetLayouts()[0]);

						if (i == mips) {
							upSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, downViews[i + 1], EGPUAccessFlags::ESRVCompute);
						}
						else {
							upSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, upViews[i + 1], EGPUAccessFlags::EUAVCompute);
						}

						upSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, downViews[i], EGPUAccessFlags::ESRVCompute);
						upSet->AddSamplerWrite(2, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
						upSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureUAV, upViews[i], EGPUAccessFlags::EUAVCompute);

						uint32_t levelWidth = outWidth >> (i + 1);
						uint32_t levelHeight = outHeight >> (i + 1);

						BloomUpSamplePushData pushData{};
						pushData.OutSize = { levelWidth, levelHeight };
						pushData.BloomStage = 0;
						pushData.FilterRadius = 0.005f;

						GRHIDevice->BindShader(cmd, m_UpSampleShader, {upSet}, &pushData);

						uint32_t groupCountX = GetComputeGroupCount(levelWidth, 32);
						uint32_t groupCountY = GetComputeGroupCount(levelHeight, 32);

						GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
						graphBuilder->BarrierRDGTexture2D(cmd, bloomUp, EGPUAccessFlags::EUAVCompute);
					}

					graphBuilder->BarrierRDGTexture2D(cmd, bloomUp, EGPUAccessFlags::ESRVCompute);
				}

				// composite
				{
					graphBuilder->BarrierRDGTexture2D(cmd, bloomComposite, EGPUAccessFlags::EUAVCompute);

					RHIBindingSet* compSet = GRDGPool->GetOrCreateBindingSet(m_UpSampleShader->GetLayouts()[0]);
					{
						compSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, upViews[0], EGPUAccessFlags::ESRVCompute);
						compSet->AddTextureWrite(1, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRV);
						compSet->AddSamplerWrite(2, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
						compSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureUAV, bloomComposite->GetTextureView(), EGPUAccessFlags::EUAVCompute);
					}

					BloomUpSamplePushData pushData{};
					pushData.OutSize = { outWidth, outHeight };
					pushData.FilterRadius = 0.005f;
					pushData.Intensity = 0.4f;
					pushData.BloomStage = 1;

					GRHIDevice->BindShader(cmd, m_UpSampleShader, {compSet}, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
					uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);

					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
					graphBuilder->BarrierRDGTexture2D(cmd, bloomComposite, EGPUAccessFlags::ECopySrc);
				}

				// copy bloom to main tex
				{
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ECopyDst);

					RHIDevice::TextureCopyRegion region{};
					region.BaseArrayLayer = 0;
					region.LayerCount = 1;
					region.MipLevel = 0;
					region.Offset = { 0.0f, 0.0f, 0.0f };

					GRHIDevice->CopyTexture(cmd, bloomComposite, region, context.OutTexture, region, {outWidth, outHeight});
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
				}
			});
	}


	ToneMapFeature::ToneMapFeature() {

		ShaderDesc desc{};
		desc.Type = EShaderType::ECompute;
		desc.Name = "ToneMap";

		m_ToneMapShader = GShaderManager->GetShaderFromCache(desc);
	}

	ToneMapFeature::~ToneMapFeature() {}

	void ToneMapFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		uint32_t outWidth = context.OutTexture->GetSizeXYZ().x;
		uint32_t outHeight = context.OutTexture->GetSizeXYZ().y;

		Texture2DDesc texDesc{};
		texDesc.Width = outWidth;
		texDesc.Height = outHeight;
		texDesc.Format = ETextureFormat::ERGBA16F;
		texDesc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc; 
		texDesc.NumMips = 1;

		RDGHandle toneMapTex = graphBuilder->CreateRDGTexture2D("ToneMap-Composite", texDesc);

		graphBuilder->AddPass(
			{toneMapTex},
			{},
			ERendererStage::EAfterPostProcessingRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* toneMap = graphBuilder->GetTextureResource(toneMapTex);
				graphBuilder->BarrierRDGTexture2D(cmd, toneMap, EGPUAccessFlags::EUAVCompute);

				RHIBindingSet* toneMapSet = GRDGPool->GetOrCreateBindingSet(m_ToneMapShader->GetLayouts()[0]);
				{
					toneMapSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRV);
					toneMapSet->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
					toneMapSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureUAV, toneMap->GetTextureView(), EGPUAccessFlags::EUAVCompute);
				}

				ToneMapPushData pushData{};
				pushData.Exposure = 5.0f;
				pushData.TexSize = { outWidth, outHeight };

				GRHIDevice->BindShader(cmd, m_ToneMapShader, {toneMapSet}, &pushData);

				uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
				uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);

				GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
				graphBuilder->BarrierRDGTexture2D(cmd, toneMap, EGPUAccessFlags::ECopySrc);

				// copy tone map to main tex
				{
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ECopyDst);

					RHIDevice::TextureCopyRegion region{};
					region.BaseArrayLayer = 0;
					region.LayerCount = 1;
					region.MipLevel = 0;
					region.Offset = { 0.0f, 0.0f, 0.0f };

					GRHIDevice->CopyTexture(cmd, toneMap, region, context.OutTexture, region, { outWidth, outHeight });
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
				}
			});
	}


	SMAAFeature::SMAAFeature() {

		SamplerDesc samplerDesc{};
		samplerDesc.Filter = ESamplerFilter::EBilinear;
		samplerDesc.AddressU = ESamplerAddress::EClamp;
		samplerDesc.AddressV = ESamplerAddress::EClamp;
		samplerDesc.AddressW = ESamplerAddress::EClamp;
		m_LinearSampler = GSamplerCache->Get(samplerDesc);

		samplerDesc.Filter = ESamplerFilter::EPoint;
		m_PointSampler = GSamplerCache->Get(samplerDesc);

		{
			Texture2DDesc desc{};
			desc.Width = 160;
			desc.Height = 560;
			desc.Format = ETextureFormat::ERG8U;
			desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
			desc.NumMips = 1;
			desc.AutoCreateSampler = false;
			//desc.PixelData = RenderUtils::LoadSMAA_AreaTex();

			m_AreaTex = new RHITexture2D(desc);
			m_AreaTex->InitRHI();
			{
				RHIDevice::SubResourceCopyRegion region{ 0, 0, 0 };

				uint8_t* data = RenderUtils::LoadSMAA_AreaTex();
				GRHIDevice->CopyDataToTexture(data, 0, m_AreaTex, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, { region }, 179200);
				delete[] data;
			}
		}
		{
			Texture2DDesc desc{};
			desc.Width = 64;
			desc.Height = 16;
			desc.Format = ETextureFormat::ER8U;
			desc.UsageFlags = ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
			desc.NumMips = 1;
			desc.AutoCreateSampler = false;
			//desc.PixelData = RenderUtils::LoadSMAA_SearchTex();

			m_SearchTex = new RHITexture2D(desc);
			m_SearchTex->InitRHI();
			{
				RHIDevice::SubResourceCopyRegion region{ 0, 0, 0 };

				uint8_t* data = RenderUtils::LoadSMAA_SearchTex();
				GRHIDevice->CopyDataToTexture(RenderUtils::LoadSMAA_SearchTex(), 0, m_SearchTex, EGPUAccessFlags::ENone, EGPUAccessFlags::ESRV, { region }, 1024);
				delete[] data;
			}
		}
		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "SMAA_Edge";

			m_EdgesShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "SMAA_Weights";

			m_WeightsShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "SMAA_Neighbors";

			m_NeighborsShader = GShaderManager->GetShaderFromCache(desc);
		}
	}

	SMAAFeature::~SMAAFeature() {

		m_AreaTex->ReleaseRHI();
		delete m_AreaTex;

		m_SearchTex->ReleaseRHI();
		delete m_SearchTex;
	}

	void SMAAFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		uint32_t outWidth = context.OutTexture->GetSizeXYZ().x;
		uint32_t outHeight = context.OutTexture->GetSizeXYZ().y;

		Texture2DDesc desc{};
		desc.Width = outWidth;
		desc.Height = outHeight;
		desc.Format = ETextureFormat::ERGBA16F;
		desc.AutoCreateSampler = false;
		//desc.SamplerDesc = m_LinearSampler->GetDesc();
		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ESampled | ETextureUsageFlags::ECopyDst;
		desc.NumMips = 1;

		RDGHandle edgesTex = graphBuilder->CreateRDGTexture2D("SMAA-Edges", desc);
		RDGHandle weightsTex = graphBuilder->CreateRDGTexture2D("SMAA-Weights", desc);

		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc;
		RDGHandle smaaCompositeTex = graphBuilder->CreateRDGTexture2D("SMAA-Composite", desc);
		RDGHandle depthTex = graphBuilder->FindRDGTexture2D("GBuffer-Depth");

		graphBuilder->AddPass(
			{ edgesTex, weightsTex, smaaCompositeTex, depthTex },
			{},
			ERendererStage::EPostProcessingRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* edges = graphBuilder->GetTextureResource(edgesTex);
				RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);

				// compute edges
				{
					graphBuilder->BarrierRDGTexture2D(cmd, edges, EGPUAccessFlags::ECopyDst);
					GRHIDevice->ClearTexture(cmd, edges, EGPUAccessFlags::ECopyDst, Vec4(0.f));
					graphBuilder->BarrierRDGTexture2D(cmd, edges, EGPUAccessFlags::EUAVCompute);

					RHIBindingSet* edgesSet = GRDGPool->GetOrCreateBindingSet(m_EdgesShader->GetLayouts()[0]);
					{
						edgesSet->AddSamplerWrite(0, 0, EShaderResourceType::ESampler, m_LinearSampler);
						edgesSet->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, m_PointSampler);
						edgesSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRV);
						edgesSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureUAV, edges->GetTextureView(), EGPUAccessFlags::EUAVCompute);
						edgesSet->AddTextureWrite(4, 0, EShaderResourceType::ETextureSRV, depth->GetTextureView(), EGPUAccessFlags::ESRV);
					}

					SMAA_EdgePushData pushData{};
					pushData.ScreenSize = { 1.f / outWidth, 1.f / outHeight, outWidth, outHeight };

					GRHIDevice->BindShader(cmd, m_EdgesShader, { edgesSet }, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
					uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);
					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);

					graphBuilder->BarrierRDGTexture2D(cmd, edges, EGPUAccessFlags::ESRVCompute);
				}

				RHITexture2D* weights = graphBuilder->GetTextureResource(weightsTex);

				// compute weights
				{
					graphBuilder->BarrierRDGTexture2D(cmd, weights, EGPUAccessFlags::ECopyDst);
					GRHIDevice->ClearTexture(cmd, weights, EGPUAccessFlags::ECopyDst, Vec4(0.f));
					graphBuilder->BarrierRDGTexture2D(cmd, weights, EGPUAccessFlags::EUAVCompute);

					RHIBindingSet* weightsSet = GRDGPool->GetOrCreateBindingSet(m_WeightsShader->GetLayouts()[0]);
					{
						weightsSet->AddSamplerWrite(0, 0, EShaderResourceType::ESampler, m_LinearSampler);
						weightsSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureSRV, edges->GetTextureView(), EGPUAccessFlags::ESRVCompute);
						weightsSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureSRV, m_AreaTex->GetTextureView(), EGPUAccessFlags::ESRV);
						weightsSet->AddTextureWrite(4, 0, EShaderResourceType::ETextureSRV, m_SearchTex->GetTextureView(), EGPUAccessFlags::ESRV);
						weightsSet->AddTextureWrite(5, 0, EShaderResourceType::ETextureUAV, weights->GetTextureView(), EGPUAccessFlags::EUAVCompute);
					}

					SMAA_WeightsPushData pushData{};
					pushData.SubSampleIndices = Vec4(0.f);
					pushData.ScreenSize = { 1.f / outWidth, 1.f / outHeight, outWidth, outHeight };

					GRHIDevice->BindShader(cmd, m_WeightsShader, { weightsSet }, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
					uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);
					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);

					graphBuilder->BarrierRDGTexture2D(cmd, weights, EGPUAccessFlags::ESRVCompute);
				}

				RHITexture2D* smaaComposite = graphBuilder->GetTextureResource(smaaCompositeTex);

				// render final image with smaa
				{
					graphBuilder->BarrierRDGTexture2D(cmd, smaaComposite, EGPUAccessFlags::EUAVCompute);

					RHIBindingSet* compSet = GRDGPool->GetOrCreateBindingSet(m_NeighborsShader->GetLayouts()[0]);
					{
						compSet->AddSamplerWrite(0, 0, EShaderResourceType::ESampler, m_LinearSampler);
						compSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRVCompute);
						compSet->AddTextureWrite(3, 0, EShaderResourceType::ETextureSRV, weights->GetTextureView(), EGPUAccessFlags::ESRV);
						compSet->AddTextureWrite(4, 0, EShaderResourceType::ETextureUAV, smaaComposite->GetTextureView(), EGPUAccessFlags::EUAVCompute);
					}

					SMAA_NeighborsPushData pushData{};
					pushData.ScreenSize = { 1.f / outWidth, 1.f / outHeight, outWidth, outHeight };

					GRHIDevice->BindShader(cmd, m_NeighborsShader, { compSet }, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
					uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);
					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);

					graphBuilder->BarrierRDGTexture2D(cmd, smaaComposite, EGPUAccessFlags::ECopySrc);
				}

				// copy smaa to main tex
				{
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ECopyDst);

					RHIDevice::TextureCopyRegion region{};
					region.BaseArrayLayer = 0;
					region.LayerCount = 1;
					region.MipLevel = 0;
					region.Offset = { 0.0f, 0.0f, 0.0f };

					GRHIDevice->CopyTexture(cmd, smaaComposite, region, context.OutTexture, region, { outWidth, outHeight });
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
				}
			});
	}


	FXAAFeature::FXAAFeature() {

		ShaderDesc desc{};
		desc.Type = EShaderType::ECompute;
		desc.Name = "FXAA";

		m_FXAAShader = GShaderManager->GetShaderFromCache(desc);
	}

	FXAAFeature::~FXAAFeature() {}

	void FXAAFeature::BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) {

		uint32_t outWidth = context.OutTexture->GetSizeXYZ().x;
		uint32_t outHeight = context.OutTexture->GetSizeXYZ().y;

		Texture2DDesc desc{};
		desc.Width = outWidth;
		desc.Height = outHeight;
		desc.Format = ETextureFormat::ERGBA16F;
		desc.UsageFlags = ETextureUsageFlags::EStorage | ETextureUsageFlags::ECopySrc;

		RDGHandle fxaaCompositeTex = graphBuilder->CreateRDGTexture2D("FXAA-Composite", desc);
		graphBuilder->AddPass(
			{ fxaaCompositeTex },
			{},
			ERendererStage::EPostProcessingRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* fxaaComposite = graphBuilder->GetTextureResource(fxaaCompositeTex);

				// compute fxaa
				{
					graphBuilder->BarrierRDGTexture2D(cmd, fxaaComposite, EGPUAccessFlags::EUAVCompute);

					RHIBindingSet* fxaaSet = GRDGPool->GetOrCreateBindingSet(m_FXAAShader->GetLayouts()[0]);
					{
						fxaaSet->AddTextureWrite(0, 0, EShaderResourceType::ETextureSRV, context.OutTexture->GetTextureView(), EGPUAccessFlags::ESRV);
						fxaaSet->AddSamplerWrite(1, 0, EShaderResourceType::ESampler, context.OutTexture->GetSampler());
						fxaaSet->AddTextureWrite(2, 0, EShaderResourceType::ETextureUAV, fxaaComposite->GetTextureView(), EGPUAccessFlags::EUAVCompute);
					}

					FXAAPushData pushData{};
					pushData.ScreenSize = { 1.f / outWidth, 1.f / outHeight, outWidth, outHeight };

					GRHIDevice->BindShader(cmd, m_FXAAShader, { fxaaSet }, &pushData);

					uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
					uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);
					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);

					graphBuilder->BarrierRDGTexture2D(cmd, fxaaComposite, EGPUAccessFlags::ECopySrc);
				}

				// copy fxaa to main tex
				{
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ECopyDst);

					RHIDevice::TextureCopyRegion region{};
					region.BaseArrayLayer = 0;
					region.LayerCount = 1;
					region.MipLevel = 0;
					region.Offset = { 0.0f, 0.0f, 0.0f };

					GRHIDevice->CopyTexture(cmd, fxaaComposite, region, context.OutTexture, region, { outWidth, outHeight });
					graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
				}
			});
	}
}