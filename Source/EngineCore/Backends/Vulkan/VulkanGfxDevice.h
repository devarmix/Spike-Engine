#pragma once

#include <Engine/Renderer/GfxDevice.h>
#include <Backends/Vulkan/VulkanDevice.h>
#include <Backends/Vulkan/VulkanSwapchain.h>

namespace Spike {

	struct VulkanImGuiTextureManager {

		void Init(uint32_t numDynamicTextures);
		void Cleanup();

		ImTextureID CreateStaticGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler);
		void DestroyStaticGuiTexture(ImTextureID id);

		ImTextureID CreateDynamicGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler, uint32_t currentFrameIndex);

		std::vector<VkDescriptorSet> DynamicTexSets[2];
		uint32_t DynamicTexSetsIndex = 0;
	};

	class VulkanRHIDevice : public RHIDevice {
	public:
		VulkanRHIDevice(Window* window, bool useImgui);
		virtual ~VulkanRHIDevice() override;

		virtual RHIData* CreateTexture2DRHI(const Texture2DDesc& desc) override;
		virtual void BarrierTexture2D(RHICommandBuffer* cmd, RHITexture2D* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;
		virtual void DestroyTexture2DRHI(RHIData* data) override;

		virtual RHIData* CreateCubeTextureRHI(const CubeTextureDesc& desc) override;
		virtual void BarrierCubeTexture(RHICommandBuffer* cmd, RHICubeTexture* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;
		virtual void DestroyCubeTextureRHI(RHIData* data) override;

		virtual void CopyTexture(RHICommandBuffer* cmd, RHITexture* src, const TextureCopyRegion& srcRegion, RHITexture* dst, 
			const TextureCopyRegion& dstRegion, Vec2Uint copySize) override;

		virtual RHIData* CreateTextureViewRHI(const TextureViewDesc& desc) override;
		virtual void DestroyTextureViewRHI(RHIData* data) override;

		virtual RHIData* CreateBufferRHI(const BufferDesc& desc) override;
		virtual void DestroyBufferRHI(RHIData* data) override;
		virtual void CopyBuffer(RHICommandBuffer* cmd, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) override;
		virtual void* MapBufferMem(RHIBuffer* buffer) override;
		virtual void BarrierBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;
		virtual void FillBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, uint32_t value, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;

		virtual RHIData* CreateBindingSetLayoutRHI(const BindingSetLayoutDesc& desc) override;
		virtual void DestroyBindingSetLayoutRHI(RHIData* data) override;

		virtual RHIData* CreateBindingSetRHI(RHIBindingSetLayout* layout) override;
		virtual void DestroyBindingSetRHI(RHIData* data) override;

		virtual RHIData* CreateShaderRHI(const ShaderDesc& desc, const ShaderCompiler::BinaryShader& binaryShader) override;
		virtual void DestroyShaderRHI(RHIData* data) override;
		virtual void BindShader(RHICommandBuffer* cmd, RHIShader* shader, RHIBindingSet* sceneDataSet) override;

		virtual RHIData* CreateSamplerRHI(const SamplerDesc& desc) override;
		virtual void DestroySamplerRHI(RHIData* data) override;

		virtual ImTextureID CreateStaticGuiTexture(RHITextureView* texture) override;
		virtual void DestroyStaticGuiTexture(ImTextureID id) override;
		virtual ImTextureID CreateDynamicGuiTexture(RHITextureView* texture) override;

		virtual RHIData* CreateCommandBufferRHI() override;
		virtual void DestroyCommandBufferRHI(RHIData* data) override;
		virtual void BeginFrameCommandBuffer(RHICommandBuffer* cmd) override;
		virtual void WaitForFrameCommandBuffer(RHICommandBuffer* cmd) override;
		virtual void ImmediateSubmit(std::function<void(RHICommandBuffer*)>&& func) override;
		virtual void DispatchCompute(RHICommandBuffer* cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		virtual void WaitGPUIdle() override;

		virtual void BeginRendering(RHICommandBuffer* cmd, const RenderInfo& info) override;
		virtual void EndRendering(RHICommandBuffer* cmd) override;
		virtual void DrawIndirectCount(RHICommandBuffer* cmd, RHIBuffer* commBuffer, size_t offset, RHIBuffer* countBuffer,
			size_t countBufferOffset, uint32_t maxDrawCount, uint32_t commStride) override;
		virtual void Draw(RHICommandBuffer* cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;

		virtual void BeginGuiNewFrame() override;
		virtual void DrawSwapchain(RHICommandBuffer* cmd, uint32_t width, uint32_t height, bool drawGui = false, RHITexture2D* fillTexture = nullptr) override;

	private:

		VulkanDevice m_Device;
		VulkanSwapchain m_Swapchain;

		VulkanImGuiTextureManager m_GuiTextureManager;
		VkDescriptorPool m_ImGuiPool;
		bool m_UsingImGui;

		VkDescriptorPool m_BindlessPool;
		VkDescriptorPool m_SceneSetPool;

		RHICommandBuffer* m_ImmCmd;
		VkFence m_ImmFence;

		struct {

			VkSemaphore SwapchainSemaphore, RenderSemaphore;
			VkFence RenderFence;
		} m_SyncObjects[2];
	};
}
