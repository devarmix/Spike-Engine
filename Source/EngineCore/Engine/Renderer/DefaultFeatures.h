#pragma once

#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	class GBufferFeature : public SceneRenderFeature {
	public:
		GBufferFeature();
		virtual ~GBufferFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) override;

	private:

		RHIShader* m_CullShader;
		RHIShader* m_HzbShader;
	};

	class DeferredLightingFeature : public SceneRenderFeature {
	public:
		DeferredLightingFeature();
		virtual ~DeferredLightingFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) override;

	private:

		RHIShader* m_LightingShader;
	};

	class SkyboxFeature : public SceneRenderFeature {
	public:
		SkyboxFeature();
		virtual ~SkyboxFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) override;

	private:

		RHIShader* m_SkyboxShader;
	};

	class SSAOFeature : public SceneRenderFeature {
	public:
		SSAOFeature();
		virtual ~SSAOFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) override;

	private:

		RHIShader* m_GenShader;
		RHIShader* m_CompositeShader;

		RHITexture2D* m_NoiseTexture;
		RHIBuffer* m_KernelBuffer;
	};

	class BloomFeature : public SceneRenderFeature {
	public:
		BloomFeature();
		virtual ~BloomFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) override;

	private:

		RHIShader* m_DownSampleShader;
		RHIShader* m_UpSampleShader;
	};

	class ToneMapFeature : public SceneRenderFeature {
	public:
		ToneMapFeature();
		virtual ~ToneMapFeature() override;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) override;

	private:

		RHIShader* m_ToneMapShader;
	};
}
