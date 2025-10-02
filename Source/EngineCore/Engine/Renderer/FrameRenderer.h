#pragma once

#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/RenderGraph.h>
#include <Engine/World/World.h>
#include <Engine/Utils/MathUtils.h>

namespace Spike {

	struct DrawIndirectCommand {

		uint32_t VertexCount;
		uint32_t InstanceCount;
		uint32_t FirstVertex;
		uint32_t FirstInstance;
	};

	struct CameraDrawData {
		Mat4x4 View;
		Mat4x4 Proj;
		Vec3 Position;

		float NearProj;
		float FarProj;
	};

	struct RenderContext {
		RHITexture2D* OutTexture;

		RHICubeTexture* EnvironmentTexture;
		RHICubeTexture* IrradianceTexture;
	};

	class RenderFeature {
	public:
		virtual ~RenderFeature() = default;
		virtual void BuildGraph(RDGBuilder* graphBuilder, const RHIWorldProxy* proxy, RenderContext context, const CameraDrawData* cameraData) = 0;
	};

	class FrameRenderer {
	public:
		FrameRenderer();
		~FrameRenderer();

		void RenderWorld(RHIWorldProxy* proxy, RenderContext context, const CameraDrawData& cameraData, std::vector<RenderFeature*>& features);
		void RenderSwapchain(uint32_t width, uint32_t height, RHITexture2D* fillTexture = nullptr);
		void BeginFrame();

		uint32_t GetFrameCount() const { return m_FrameCount; }
		RHITexture2D* GetBRDFLut() { return m_BRDFLut; }

		void SubmitToFrameQueue(std::function<void()>&& func) { m_FrameQueues[m_FrameCount % 2].Push(std::move(func)); }

	private:
		void FlushFrameQueue(uint32_t idx);
		void UpdateFontTexture(uint8_t** outData);

	private:
		RHICommandBuffer* m_CommandBuffers[2];
		RHIDevice::ImGuiRTState m_GuiRTStates[2];

		ThreadSafeQueue<std::function<void()>> m_FrameQueues[2];

		RHITexture2D* m_BRDFLut;
		RHITexture2D* m_GuiFontTexture;
		uint32_t m_FrameCount;
	};

	// global frame renderer pointer
	extern FrameRenderer* GFrameRenderer;
}