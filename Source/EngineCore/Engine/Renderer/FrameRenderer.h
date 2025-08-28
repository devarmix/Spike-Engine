#pragma once

#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/RenderGraph.h>

namespace Spike {

	struct DrawIndirectCommand {

		uint32_t VertexCount;
		uint32_t InstanceCount;
		uint32_t FirstVertex;
		uint32_t FirstInstance;
	};

	struct FrameData {

		RHIBuffer* LightsBuffer;
		RHIBuffer* DrawCommandsBuffer;
		RHIBuffer* DrawCountsBuffer;
		RHIBuffer* SceneObjectsBuffer;
		RHIBuffer* BatchOffsetsBuffer;
		RHIBuffer* VisibilityBuffer;

		uint32_t ObjectsOffset;
		uint32_t DrawCountOffset;
		uint32_t LightsOffset;
	};

	struct DrawBatch {

		uint32_t CommandsCount;
		RHIShader* Shader;
	};

	struct alignas(16) SceneObjectGPUData {

		Vec4 BoundsOrigin; // w - bounds radius
		Vec4 BoundsExtents;
		Mat4x4 GlobalTransform;
		Mat4x4 InverseTransform;

		uint32_t FirstIndex;
		uint32_t IndexCount;

		uint64_t IndexBufferAddress;
		uint64_t VertexBufferAddress;

		uint32_t MaterialBufferIndex;
		uint32_t DrawBatchID;

		int LastVisibilityIndex;
		int CurrentVisibilityIndex;

		float Padding0[2];
	};

	struct alignas(16) SceneLightGPUData {

		Vec4 Position;
		Vec4 Direction; // for directional and spot
		Vec4 Color;

		float Intensity;
		int Type; // 0 = directional, 1 = point, 2 = spot
		float InnerConeCos;
		float OuterConeCos;
	};

	struct alignas(16) SceneGPUData {

		Mat4x4 View;
		Mat4x4 Proj;
		Mat4x4 InverseView;
		Mat4x4 InverseProj;
		Mat4x4 ViewProj;

		Vec4 CameraPos;
		Vec4 FrustumPlanes[6];

		float P00;
		float P11;
		float NearProj;
		float FarProj;

		uint32_t LightsCount;
		uint32_t ObjectsCount;

		float Padding0[2];
		Vec4 Padding1[3];
	};

	struct RenderContext {

		RHITexture2D* OutTexture;

		RHICubeTexture* EnvironmentTexture;
		RHICubeTexture* IrradianceTexture;
	};

	struct SceneRenderProxy {

		std::vector<SceneObjectGPUData> Objects;
		std::vector<SceneLightGPUData> Lights;
		std::vector<DrawBatch> Batches;

		struct {
			Mat4x4 View;
			Mat4x4 Proj;
			Vec4 Position;

			float NearProj;
			float FarProj;
		} CameraData;
	};

	class SceneRenderFeature {
	public:
		virtual ~SceneRenderFeature() = default;

		virtual void BuildGraph(RDGBuilder* graphBuilder, const SceneRenderProxy* scene, RenderContext context) = 0;
	};

	constexpr uint32_t MAX_SCENE_DRAWS_PER_FRAME = 8;
	constexpr uint32_t MAX_DRAWS_PER_FRAME = 100000;
	constexpr uint32_t MAX_LIGHTS_PER_FRAME = 5000;
	constexpr uint32_t MAX_SHADERS_PER_FRAME = 100;

	class FrameRenderer {
	public:
		FrameRenderer();
		~FrameRenderer();

		void RenderScene(const SceneRenderProxy& scene, RenderContext& context, std::vector<SceneRenderFeature*>& features);
		void RenderSwapchain(uint32_t width, uint32_t height, RHITexture2D* fillTexture = nullptr);
		void BeginFrame();

		uint32_t GetFrameCount() const { return m_FrameCount; }
		RHITexture2D* GetBRDFLut() { return m_BRDFLut; }

		FrameData& GetFrameData() { return m_FrameData[m_FrameCount % 2]; }
		FrameData& GetPrevFrameData() { return m_FrameData[(m_FrameCount + 1) % 2]; }

		void PushToExecQueue(std::function<void()>&& func) { m_ExecQueue[m_FrameCount % 2].PushFunction(std::move(func)); }

	private:

		FrameData m_FrameData[2];
		RHICommandBuffer* m_CommandBuffers[2];

		struct FuncQueue {

			std::deque<std::function<void()>> Queue;

			void PushFunction(std::function<void()>&& function);
			void Flush();
		} m_ExecQueue[2];

		RHITexture2D* m_BRDFLut;
		uint32_t m_FrameCount;
	};

	// global frame renderer pointer
	extern FrameRenderer* GFrameRenderer;
}