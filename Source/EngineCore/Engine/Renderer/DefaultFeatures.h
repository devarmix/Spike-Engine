#pragma once

#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	enum class EFeatureType : uint8_t {
		ENone = 0,

		EGBuffer,
		EDeferredLightning,
		ESkybox,
		ESSAO,
		EFXAA,
		ESMAA,
		EBloom,
		EToneMap
	};

	class GBufferFeature : public RenderFeature {
	public:
		GBufferFeature();
		virtual ~GBufferFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_CullShader;
		RHIShader* m_HzbShader;
	};

	class DeferredLightingFeature : public RenderFeature {
	public:
		DeferredLightingFeature();
		virtual ~DeferredLightingFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_LightingShader;
	};

	class SkyboxFeature : public RenderFeature {
	public:
		SkyboxFeature();
		virtual ~SkyboxFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_SkyboxShader;
	};

	class SSAOFeature : public RenderFeature {
	public:
		SSAOFeature();
		virtual ~SSAOFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_GenShader;
		RHIShader* m_CompositeShader;

		RHITexture2D* m_NoiseTexture;
		RHIBuffer* m_KernelBuffer;
	};

	class BloomFeature : public RenderFeature {
	public:
		BloomFeature();
		virtual ~BloomFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_DownSampleShader;
		RHIShader* m_UpSampleShader;
	};

	class ToneMapFeature : public RenderFeature {
	public:
		ToneMapFeature();
		virtual ~ToneMapFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_ToneMapShader;
	};

	class SMAAFeature : public RenderFeature {
	public:
		SMAAFeature();
		virtual ~SMAAFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHITexture2D* m_AreaTex;
		RHITexture2D* m_SearchTex;

		RHISampler* m_LinearSampler;
		RHISampler* m_PointSampler;

		RHIShader* m_EdgesShader;
		RHIShader* m_WeightsShader;
		RHIShader* m_NeighborsShader;
	};

	class FXAAFeature : public RenderFeature {
	public:
		FXAAFeature();
		virtual ~FXAAFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) override;

	private:

		RHIShader* m_FXAAShader;
	};
}