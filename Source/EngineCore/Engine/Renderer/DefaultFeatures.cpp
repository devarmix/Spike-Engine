#include <Engine/Renderer/DefaultFeatures.h>
#include <Engine/Core/Log.h>

#include <Generated/GeneratedIndirectCull.h>
#include <Generated/GeneratedDeferredPBR.h>
#include <Generated/GeneratedDepthPyramid.h>
#include <Generated/GeneratedDeferredLighting.h>
#include <Generated/GeneratedSSAO.h>
#include <Generated/GeneratedBloom.h>
#include <Generated/GeneratedToneMap.h>
#include <Generated/GeneratedSkybox.h>

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

	void GBufferFeature::BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context, RHIBindingSet* sceneDataSet) {

		FrameData& frameData = GFrameRenderer->GetFrameData();
		FrameData& prevFrameData = GFrameRenderer->GetPrevFrameData();

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
		uboDesc.Size = sizeof(SceneGPUData);
		uboDesc.UsageFlags = EBufferUsageFlags::EConstant;
		uboDesc.MemUsage = EBufferMemUsage::ECPUToGPU;
		RDGHandle sceneUBO = graphBuilder->CreateRDGBuffer("Scene-UBO", uboDesc);

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

		graphBuilder->RegisterExternalBuffer(frameData.SceneObjectsBuffer);
		graphBuilder->RegisterExternalBuffer(frameData.DrawCommandsBuffer);
		graphBuilder->RegisterExternalBuffer(frameData.DrawCountsBuffer);
		graphBuilder->RegisterExternalBuffer(frameData.BatchOffsetsBuffer);
		graphBuilder->RegisterExternalBuffer(frameData.VisibilityBuffer);
		graphBuilder->RegisterExternalBuffer(prevFrameData.VisibilityBuffer);

		graphBuilder->AddPass(
			{ albedoTex, normalTex, materialTex, depthTex, hzbTex },
			{ sceneUBO }, 
			ERendererStage::EOpaqueRender, 
			[=, this](RHICommandBuffer* cmd) {

				RHIBuffer* ubo = graphBuilder->GetBufferResource(sceneUBO);
				{
					ENGINE_INFO("{0}, {1}, {2}", scene->CameraData.Position.x, scene->CameraData.Position.y, scene->CameraData.Position.z);

					// Camera settings
					glm::vec3 camPos = glm::vec3(0.0f, 0.0f, -3.0f);
					glm::vec3 camTarget = glm::vec3(0.0f, 0.0f, 0.0f);
					glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

					// View matrix
					glm::mat4 view = glm::lookAt(camPos, camTarget, camUp);

					SceneGPUData* sceneData = (SceneGPUData*)ubo->GetMappedData();
					sceneData[0].View = scene->CameraData.View;
					sceneData[0].Proj = scene->CameraData.Proj;
					sceneData[0].ViewProj = scene->CameraData.Proj * scene->CameraData.View;
					sceneData[0].InverseProj = glm::inverse(scene->CameraData.Proj);
					sceneData[0].InverseView = glm::inverse(scene->CameraData.View);
					sceneData[0].CameraPos = scene->CameraData.Position;

					glm::mat4& vp = sceneData[0].ViewProj;
					sceneData[0].FrustumPlanes[0] = glm::vec4(
						vp[0][3] + vp[0][0], vp[1][3] + vp[1][0],
						vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
					sceneData[0].FrustumPlanes[1] = glm::vec4(
						vp[0][3] - vp[0][0], vp[1][3] - vp[1][0],
						vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
					sceneData[0].FrustumPlanes[2] = glm::vec4(
						vp[0][3] + vp[0][1], vp[1][3] + vp[1][1],
						vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
					sceneData[0].FrustumPlanes[3] = glm::vec4(
						vp[0][3] - vp[0][1], vp[1][3] - vp[1][1],
						vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
					sceneData[0].FrustumPlanes[4] = glm::vec4(
						vp[0][3] + vp[0][2], vp[1][3] + vp[1][2],
						vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
					sceneData[0].FrustumPlanes[5] = glm::vec4(
						vp[0][3] - vp[0][2], vp[1][3] - vp[1][2],
						vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

					for (int i = 0; i < 6; i++) {

						float length = glm::length(glm::vec3(sceneData[0].FrustumPlanes[i]));
						sceneData[0].FrustumPlanes[i] /= length;
					}

					sceneData[0].P00 = sceneData[0].Proj[0][0];
					sceneData[0].P11 = sceneData[0].Proj[1][1];
					sceneData[0].NearProj = scene->CameraData.NearProj;
					sceneData[0].FarProj = scene->CameraData.FarProj;
					sceneData[0].LightsOffset = frameData.LightsOffset;
					sceneData[0].ObjectsOffset = frameData.ObjectsOffset;
					sceneData[0].DrawCountOffset = frameData.DrawCountOffset;
					sceneData[0].LightsCount = (uint32_t)scene->Lights.size();
					sceneData[0].ObjectsCount = (uint32_t)scene->Objects.size();

					sceneDataSet->AddResourceWrite({ .Slot = 0, .ArrayElement = 0, .Type = EShaderResourceType::EConstantBuffer, .Buffer = ubo, .BufferRange = sizeof(SceneGPUData), .BufferOffset = 0 });
				}

				memcpy((SceneObjectGPUData*)frameData.SceneObjectsBuffer->GetMappedData() + frameData.ObjectsOffset, 
					scene->Objects.data(), sizeof(SceneObjectGPUData) * scene->Objects.size());

				std::vector<uint32_t> batchOffsets;
				batchOffsets.resize(scene->Batches.size());
				{
					uint32_t* offsetsBuffer = (uint32_t*)frameData.BatchOffsetsBuffer->GetMappedData();

					for (int i = 0; i < scene->Batches.size(); i++) {

						if (i > 0) {
							batchOffsets[i] = scene->Batches[i - 1].CommandsCount + batchOffsets[i - 1];
						}
						else {
							batchOffsets[i] = 0;
						}

						offsetsBuffer[frameData.DrawCountOffset + i] = batchOffsets[i];
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

				auto cullGeometry = [=, this](bool prepass) {

					m_CullShader->SetBufferUAV(IndirectCullResources.DrawCommandsBuffer, frameData.DrawCommandsBuffer);
					m_CullShader->SetBufferUAV(IndirectCullResources.DrawCountsBuffer, frameData.DrawCountsBuffer);
					m_CullShader->SetBufferSRV(IndirectCullResources.BatchOffsetsBuffer, frameData.BatchOffsetsBuffer);
					m_CullShader->SetBufferSRV(IndirectCullResources.LastVisibilityBuffer, prevFrameData.VisibilityBuffer);
					m_CullShader->SetBufferSRV(IndirectCullResources.SceneObjectsBuffer, frameData.SceneObjectsBuffer);
					m_CullShader->SetBufferUAV(IndirectCullResources.CurrentVisibilityBuffer, frameData.VisibilityBuffer);
					m_CullShader->SetUint(IndirectCullResources.IsPrepass, prepass ? 1 : 0);

					if (!prepass) {

						m_CullShader->SetTextureSRV(IndirectCullResources.DepthPyramid, hzb->GetTextureView());
						m_CullShader->SetUint(IndirectCullResources.PyramidSize, hzbSize);
						m_CullShader->SetFloat(IndirectCullResources.CullZNear, scene->CameraData.Proj[3][2]);
					}

					GRHIDevice->BindShader(cmd, m_CullShader, sceneDataSet);

					uint32_t groupSize = uint32_t((scene->Objects.size() / 256) + 1);
					GRHIDevice->DispatchCompute(cmd, groupSize, 1, 1);

					graphBuilder->BarrierRDGBuffer(cmd, frameData.DrawCommandsBuffer, sizeof(DrawIndirectCommand) * scene->Objects.size(),
						sizeof(DrawIndirectCommand) * frameData.ObjectsOffset, EGPUAccessFlags::EIndirectArgs);
					graphBuilder->BarrierRDGBuffer(cmd, frameData.DrawCountsBuffer, sizeof(uint32_t) * scene->Batches.size(),
						sizeof(uint32_t) * frameData.DrawCountOffset, EGPUAccessFlags::EIndirectArgs);
					};

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

					for (int i = 0; i < scene->Batches.size(); i++) {

						DrawBatch currBatch = scene->Batches[i];

						currBatch.Shader->SetBufferSRV(DeferredPBRResources.SceneObjectsBuffer, frameData.SceneObjectsBuffer);
						GRHIDevice->BindShader(cmd, currBatch.Shader, sceneDataSet);

						uint32_t stride = sizeof(DrawIndirectCommand);
						uint64_t commandOffset = frameData.ObjectsOffset + batchOffsets[i];
						uint64_t countOffset = frameData.DrawCountOffset + i;

						GRHIDevice->DrawIndirectCount(cmd, frameData.DrawCommandsBuffer, stride * commandOffset, frameData.DrawCountsBuffer,
							sizeof(uint32_t) * countOffset, (uint32_t)scene->Objects.size(), stride);
					}

					GRHIDevice->EndRendering(cmd);
					};

				// first cull pass
				{
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

						m_HzbShader->SetTextureUAV(DepthPyramidResources.OutImage, hzbViews[i]);
						if (i == 0) {
							m_HzbShader->SetTextureSRV(DepthPyramidResources.InImage, depth->GetTextureView());
						}
						else {
							m_HzbShader->SetTextureUAVReadOnly(DepthPyramidResources.InImage, hzbViews[i - 1]);
						} 

						uint32_t levelSize = hzbSize >> i;
						m_HzbShader->SetUint(DepthPyramidResources.MipSize, levelSize);

						GRHIDevice->BindShader(cmd, m_HzbShader);

						uint32_t groupCount = GetComputeGroupCount(levelSize, 32);
						GRHIDevice->DispatchCompute(cmd, groupCount, groupCount, 1);

						graphBuilder->BarrierRDGTexture2D(cmd, hzb, EGPUAccessFlags::EUAVCompute);
					}

					graphBuilder->BarrierRDGTexture2D(cmd, hzb, EGPUAccessFlags::ESRV);
				}

				// second cull pass
				{
					graphBuilder->FillRDGBuffer(cmd, frameData.DrawCountsBuffer, sizeof(uint32_t) * scene->Batches.size(),
						sizeof(uint32_t) * frameData.DrawCountOffset, 0, EGPUAccessFlags::EUAVCompute);
					graphBuilder->BarrierRDGBuffer(cmd, frameData.DrawCommandsBuffer, sizeof(DrawIndirectCommand) * scene->Objects.size(),
						sizeof(DrawIndirectCommand) * frameData.ObjectsOffset, EGPUAccessFlags::EUAVCompute);

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

	void DeferredLightingFeature::BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context, RHIBindingSet* sceneDataSet) {

		FrameData& frameData = GFrameRenderer->GetFrameData();

		RDGHandle albedoTex = graphBuilder->FindRDGTexture2D("GBuffer-Albedo");
		RDGHandle normalTex = graphBuilder->FindRDGTexture2D("GBuffer-Normal");
		RDGHandle materialTex = graphBuilder->FindRDGTexture2D("GBuffer-Material");
		RDGHandle depthTex = graphBuilder->FindRDGTexture2D("GBuffer-Depth");

		graphBuilder->RegisterExternalTexture2D(context.OutTexture);
		graphBuilder->RegisterExternalBuffer(frameData.LightsBuffer);

		graphBuilder->AddPass(
			{albedoTex, normalTex, materialTex, depthTex},
			{},
			ERendererStage::ELightsRender,
			[=, this](RHICommandBuffer* cmd) {

				memcpy((SceneLightGPUData*)frameData.LightsBuffer->GetMappedData() + frameData.LightsOffset, scene->Lights.data(),
					sizeof(SceneLightGPUData) * scene->Lights.size());

				RHITexture2D* albedo = graphBuilder->GetTextureResource(albedoTex);
				RHITexture2D* normal = graphBuilder->GetTextureResource(normalTex);
				RHITexture2D* material = graphBuilder->GetTextureResource(materialTex);
				RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);
				RHITexture2D* brdf = GFrameRenderer->GetBRDFLut();

				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::EColorTarget);

				m_LightingShader->SetTextureSRV(DeferredLightingResources.AlbedoMap, albedo->GetTextureView());
				m_LightingShader->SetTextureSRV(DeferredLightingResources.NormalMap, normal->GetTextureView());
			    m_LightingShader->SetTextureSRV(DeferredLightingResources.MaterialMap, material->GetTextureView());
				m_LightingShader->SetTextureSRV(DeferredLightingResources.DepthMap, depth->GetTextureView());
				m_LightingShader->SetTextureSRV(DeferredLightingResources.EnvMap, context.EnvironmentTexture->GetTextureView());
				m_LightingShader->SetTextureSRV(DeferredLightingResources.IrrMap, context.IrradianceTexture->GetTextureView());
				m_LightingShader->SetTextureSRV(DeferredLightingResources.BRDFMap, brdf->GetTextureView());
				m_LightingShader->SetBufferSRV(DeferredLightingResources.LightsBuffer, frameData.LightsBuffer);
				m_LightingShader->SetUint(DeferredLightingResources.EnvMapNumMips, context.EnvironmentTexture->GetNumMips());

				Vec4 colorClear = { 0.0f, 0.0f, 0.0f, 1.0f };

				RHIDevice::RenderInfo info{};
				info.ColorTargets = { context.OutTexture->GetTextureView() };
				info.DepthTarget = nullptr;
				info.ColorClear = &colorClear;
				info.DepthClear = nullptr;
				info.DrawSize = { context.OutTexture->GetSizeXYZ().x, context.OutTexture->GetSizeXYZ().y };

				GRHIDevice->BeginRendering(cmd, info);
				GRHIDevice->BindShader(cmd, m_LightingShader, sceneDataSet);
				GRHIDevice->Draw(cmd, 3, 1, 0, 0);
				GRHIDevice->EndRendering(cmd);

				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
			});
	}



	SkyboxFeature::SkyboxFeature() {

		ShaderDesc desc{};
		desc.Type = EShaderType::EGraphics;
		desc.Name = "Skybox";
		desc.EnableDepthTest = true;

		m_SkyboxShader = GShaderManager->GetShaderFromCache(desc);
	}

	SkyboxFeature::~SkyboxFeature() {}

	void SkyboxFeature::BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context, RHIBindingSet* sceneDataSet) {

		RDGHandle depthTex = graphBuilder->FindRDGTexture2D("GBuffer-Depth");
		
		graphBuilder->AddPass(
			{ depthTex },
			{},
			ERendererStage::EAfterLightsRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);
				graphBuilder->BarrierRDGTexture2D(cmd, depth, EGPUAccessFlags::EDepthTarget);
				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::EColorTarget);

				m_SkyboxShader->SetTextureSRV(SkyboxResources.SkyboxMap, context.EnvironmentTexture->GetTextureView());

				RHIDevice::RenderInfo info{};
				info.ColorTargets = { context.OutTexture->GetTextureView() };
				info.DepthTarget = depth->GetTextureView();
				info.ColorClear = nullptr;
				info.DepthClear = nullptr;
				info.DrawSize = { context.OutTexture->GetSizeXYZ().x, context.OutTexture->GetSizeXYZ().y };

				GRHIDevice->BindShader(cmd, m_SkyboxShader, sceneDataSet);
				GRHIDevice->BeginRendering(cmd, info);
				GRHIDevice->Draw(cmd, 3, 1, 0, 0);
				GRHIDevice->EndRendering(cmd);

				graphBuilder->BarrierRDGTexture2D(cmd, context.OutTexture, EGPUAccessFlags::ESRV);
			});
	}


	SSAOFeature::SSAOFeature() {

		{
			ShaderDesc desc{};
			desc.Type = EShaderType::ECompute;
			desc.Name = "SSAO";

			m_SSAOShader = GShaderManager->GetShaderFromCache(desc);
		}
		{
			std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
			std::default_random_engine generator;

			Vec4* ssaoNoise = (Vec4*)malloc(sizeof(Vec4) * 16);

			for (int i = 0; i < 16; i++)
			{
				glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
				ssaoNoise[i] = glm::vec4(noise, 1.0f);
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
			desc.PixelData = ssaoNoise;

			m_NoiseTexture = new RHITexture2D(desc);
			m_NoiseTexture->InitRHI();
		}
	}

	SSAOFeature::~SSAOFeature() {

		m_NoiseTexture->ReleaseRHI();
		delete m_NoiseTexture;
	}

	void SSAOFeature::BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context, RHIBindingSet* sceneDataSet) {

		FrameData& frameData = GFrameRenderer->GetFrameData();

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

		graphBuilder->AddPass(
			{ssaoTex, ssaoCompositeTex },
			{},
			ERendererStage::EPostProcessingRender,
			[=, this](RHICommandBuffer* cmd) {

				RHITexture2D* ssao = graphBuilder->GetTextureResource(ssaoTex);
				RHITexture2D* ssaoComposite = graphBuilder->GetTextureResource(ssaoCompositeTex);

				// gen ssao
				{
					graphBuilder->BarrierRDGTexture2D(cmd, ssao, EGPUAccessFlags::EUAVCompute);

					RHITexture2D* normal = graphBuilder->GetTextureResource(normalTex);
					RHITexture2D* depth = graphBuilder->GetTextureResource(depthTex);

					m_SSAOShader->SetTextureSRV(SSAOResources.SampledTextureA, depth->GetTextureView());
					m_SSAOShader->SetTextureSRV(SSAOResources.SampledTextureB, normal->GetTextureView());
					m_SSAOShader->SetTextureSRV(SSAOResources.NoiseMap, m_NoiseTexture->GetTextureView());
					m_SSAOShader->SetTextureUAV(SSAOResources.OutTexture, ssao->GetTextureView());
					m_SSAOShader->SetFloat(SSAOResources.Radius, 0.5f);
					m_SSAOShader->SetFloat(SSAOResources.Bias, 0.025f);
					m_SSAOShader->SetUint(SSAOResources.NumSamples, 32);
					m_SSAOShader->SetFloat(SSAOResources.Intensity, 1.5f);
					m_SSAOShader->SetUint(SSAOResources.SSAOStage, 0);
					m_SSAOShader->SetVec2(SSAOResources.TexSize, { ssao->GetSizeXYZ().x,  ssao->GetSizeXYZ().y});

					GRHIDevice->BindShader(cmd, m_SSAOShader, sceneDataSet);

					uint32_t groupCountX = GetComputeGroupCount(ssao->GetSizeXYZ().x, 32);
					uint32_t groupCountY = GetComputeGroupCount(ssao->GetSizeXYZ().y, 32);

					GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
					graphBuilder->BarrierRDGTexture2D(cmd, ssao, EGPUAccessFlags::ESRVCompute);
				}

				// composite
				{
					graphBuilder->BarrierRDGTexture2D(cmd, ssaoComposite, EGPUAccessFlags::EUAVCompute);

					m_SSAOShader->SetTextureSRV(SSAOResources.SampledTextureA, ssao->GetTextureView());
					m_SSAOShader->SetTextureSRV(SSAOResources.SampledTextureB, context.OutTexture->GetTextureView());
					m_SSAOShader->SetTextureUAV(SSAOResources.OutTexture, ssaoComposite->GetTextureView());
					m_SSAOShader->SetUint(SSAOResources.SSAOStage, 1);
					m_SSAOShader->SetVec2(SSAOResources.TexSize, { ssaoComposite->GetSizeXYZ().x,  ssaoComposite->GetSizeXYZ().y });

					GRHIDevice->BindShader(cmd, m_SSAOShader, sceneDataSet);

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

		ShaderDesc desc{};
		desc.Type = EShaderType::ECompute;
		desc.Name = "Bloom";

		m_BloomShader = GShaderManager->GetShaderFromCache(desc);
	}

	BloomFeature::~BloomFeature() {}

	void BloomFeature::BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context, RHIBindingSet* sceneDataSet) {

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

						if (i == 0) {
							m_BloomShader->SetTextureSRV(BloomResources.SampledTextureA, context.OutTexture->GetTextureView());
						}
						else {
							m_BloomShader->SetTextureUAVReadOnly(BloomResources.SampledTextureA, downViews[i - 1]);
						}
						m_BloomShader->SetTextureUAV(BloomResources.OutTexture, downViews[i]);

						uint32_t srcWidth = outWidth >> i;
						uint32_t srcHeight = outHeight >> i;

						uint32_t levelWidth = srcWidth >> 1;
						uint32_t levelHeight = srcHeight >> 1;

						m_BloomShader->SetVec2(BloomResources.SrcSize, { srcWidth, srcHeight });
						m_BloomShader->SetVec2(BloomResources.OutSize, { levelWidth, levelHeight });
						m_BloomShader->SetUint(BloomResources.MipLevel, i);
						m_BloomShader->SetFloat(BloomResources.Threadshold, 2.0f);
						m_BloomShader->SetFloat(BloomResources.SoftThreadshold, 0.5f);
						m_BloomShader->SetUint(BloomResources.BloomStage, 0);

						GRHIDevice->BindShader(cmd, m_BloomShader);

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

						if (i == mips) {
							m_BloomShader->SetTextureSRV(BloomResources.SampledTextureA, downViews[i + 1]);
						}
						else {
							m_BloomShader->SetTextureUAVReadOnly(BloomResources.SampledTextureA, upViews[i + 1]);
						}

						m_BloomShader->SetTextureSRV(BloomResources.SampledTextureB, downViews[i]);
						m_BloomShader->SetTextureUAV(BloomResources.OutTexture, upViews[i]);

						uint32_t levelWidth = outWidth >> (i + 1);
						uint32_t levelHeight = outHeight >> (i + 1);

						m_BloomShader->SetVec2(BloomResources.OutSize, { levelWidth, levelHeight });
						m_BloomShader->SetFloat(BloomResources.FilterRadius, 0.005f);
						m_BloomShader->SetUint(BloomResources.BloomStage, 1);

						GRHIDevice->BindShader(cmd, m_BloomShader);

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

					m_BloomShader->SetTextureSRV(BloomResources.SampledTextureA, upViews[0]);
					m_BloomShader->SetTextureSRV(BloomResources.SampledTextureB, context.OutTexture->GetTextureView());
					m_BloomShader->SetTextureUAV(BloomResources.OutTexture, bloomComposite->GetTextureView());
					m_BloomShader->SetVec2(BloomResources.OutSize, { outWidth, outHeight });
					m_BloomShader->SetFloat(BloomResources.FilterRadius, 0.005f);
					m_BloomShader->SetFloat(BloomResources.BloomIntensity, 0.4f);
					m_BloomShader->SetUint(BloomResources.BloomStage, 2);

					GRHIDevice->BindShader(cmd, m_BloomShader);

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

	void ToneMapFeature::BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context, RHIBindingSet* sceneDataSet) {

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
				m_ToneMapShader->SetTextureSRV(ToneMapResources.InTexture, context.OutTexture->GetTextureView());
				m_ToneMapShader->SetTextureUAV(ToneMapResources.OutTexture, toneMap->GetTextureView());
				m_ToneMapShader->SetFloat(ToneMapResources.Exposure, 3.0f);
				m_ToneMapShader->SetVec2(ToneMapResources.TexSize, { outWidth, outHeight });

				GRHIDevice->BindShader(cmd, m_ToneMapShader);

				uint32_t groupCountX = GetComputeGroupCount(outWidth, 32);
				uint32_t groupCountY = GetComputeGroupCount(outHeight, 32);

				GRHIDevice->DispatchCompute(cmd, groupCountX, groupCountY, 1);
				graphBuilder->BarrierRDGTexture2D(cmd, toneMap, EGPUAccessFlags::ECopySrc);

				// copy bloom to main tex
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
}