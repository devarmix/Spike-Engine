#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Core/Window.h>
#include <Engine/Renderer/CubeTexture.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/RenderGraph.h>
#include <Engine/Multithreading/RenderThread.h>

#include <imgui.h>

namespace Spike {

	// utility function
	uint32_t RoundUpToPowerOfTwo(float value);
	uint32_t GetComputeGroupCount(uint32_t threadCount, uint32_t groupSize);

	class RHICommandBuffer : public RHIResource {
	public:
		RHICommandBuffer() : m_RHIData(nullptr) {}
		virtual ~RHICommandBuffer() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		RHIData* GetRHIData() { return m_RHIData; }

	private:

		RHIData* m_RHIData;
	};

	class RHIDevice {
	public:
		virtual ~RHIDevice() = default;

		static void Create(Window* window, bool useImGui = false);

		virtual RHIData* CreateTexture2DRHI(const Texture2DDesc& desc) = 0;
		virtual void BarrierTexture2D(RHICommandBuffer* cmd, RHITexture2D* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) = 0;
		virtual void DestroyTexture2DRHI(RHIData* data) = 0;

		virtual RHIData* CreateCubeTextureRHI(const CubeTextureDesc& desc) = 0;
		virtual void BarrierCubeTexture(RHICommandBuffer* cmd, RHICubeTexture* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) = 0;
		virtual void DestroyCubeTextureRHI(RHIData* data) = 0;

		struct TextureCopyRegion {

			uint32_t BaseArrayLayer;
			uint32_t MipLevel;
			uint32_t LayerCount;
			Vec3Int Offset;
		};

		virtual void CopyTexture(RHICommandBuffer* cmd, RHITexture* src, const TextureCopyRegion& srcRegion, RHITexture* dst, const TextureCopyRegion& dstRegion, Vec2Uint copySize) = 0;

		virtual RHIData* CreateTextureViewRHI(const TextureViewDesc& desc) = 0;
		virtual void DestroyTextureViewRHI(RHIData* data) = 0;

		virtual RHIData* CreateBufferRHI(const BufferDesc& desc) = 0;
		virtual void DestroyBufferRHI(RHIData* data) = 0;
		virtual void CopyBuffer(RHICommandBuffer* cmd, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) = 0;
		virtual void* MapBufferMem(RHIBuffer* buffer) = 0;
		virtual void BarrierBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) = 0;
		virtual void FillBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, uint32_t value, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) = 0;
		virtual uint64_t GetBufferGPUAddress(RHIBuffer* buffer) = 0;

		virtual RHIData* CreateBindingSetLayoutRHI(const BindingSetLayoutDesc& desc) = 0;
		virtual void DestroyBindingSetLayoutRHI(RHIData* data) = 0;

		virtual RHIData* CreateBindingSetRHI(RHIBindingSetLayout* layout) = 0;
		virtual void DestroyBindingSetRHI(RHIData* data) = 0;

		virtual RHIData* CreateShaderRHI(const ShaderDesc& desc, const ShaderCompiler::BinaryShader& binaryShader, const std::vector<RHIBindingSetLayout*>& layouts) = 0;
		virtual void DestroyShaderRHI(RHIData* data) = 0;
		virtual void BindShader(RHICommandBuffer* cmd, RHIShader* shader, std::vector<RHIBindingSet*> shaderSets = {}, void* pushData = nullptr) = 0;

		virtual RHIData* CreateSamplerRHI(const SamplerDesc& desc) = 0;
		virtual void DestroySamplerRHI(RHIData* data) = 0;

		virtual ImTextureID CreateStaticGuiTexture(RHITextureView* texture) = 0;
		virtual void DestroyStaticGuiTexture(ImTextureID id) = 0;
		virtual ImTextureID CreateDynamicGuiTexture(RHITextureView* texture) = 0;

		virtual RHIData* CreateCommandBufferRHI() = 0;
		virtual void DestroyCommandBufferRHI(RHIData* data) = 0;
		virtual void BeginFrameCommandBuffer(RHICommandBuffer* cmd) = 0;
		virtual void WaitForFrameCommandBuffer(RHICommandBuffer* cmd) = 0;
		virtual void ImmediateSubmit(std::function<void(RHICommandBuffer*)>&& func) = 0;
		virtual void DispatchCompute(RHICommandBuffer* cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void WaitGPUIdle() = 0;

		struct RenderInfo {

			std::vector<RHITextureView*> ColorTargets;
			RHITextureView* DepthTarget;

			Vec4* ColorClear;
			Vec2* DepthClear;

			Vec2Uint DrawSize;
		};

		virtual void BeginRendering(RHICommandBuffer* cmd, const RenderInfo& info) = 0;
		virtual void EndRendering(RHICommandBuffer* cmd) = 0;
		virtual void DrawIndirectCount(RHICommandBuffer* cmd, RHIBuffer* commBuffer, size_t offset, RHIBuffer* countBuffer,
			size_t countBufferOffset, uint32_t maxDrawCount, uint32_t commStride) = 0;
		virtual void Draw(RHICommandBuffer* cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;

		virtual void BeginGuiNewFrame() = 0;
		virtual void DrawSwapchain(RHICommandBuffer* cmd, uint32_t width, uint32_t height, bool drawGui = false, RHITexture2D* fillTexture = nullptr) = 0;

	public:

		Ref<Texture2D> DefErrorTexture2D;
	};

	// global rhi device pointer
	extern RHIDevice* GRHIDevice;
}
