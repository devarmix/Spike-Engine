#pragma once

#include <Engine/Renderer/GfxDevice.h>
#include <Backends/Vulkan/VulkanDevice.h>
#include <Backends/Vulkan/VulkanSwapchain.h>
#include <Backends/Vulkan/VulkanResources.h>

namespace Spike {

	struct VulkanImGuiTextureManager {

		void Init();
		void Cleanup();

		VkDescriptorSet GetTextureSet(VkImageView view, VkSampler sampler, uint32_t currentFrameIndex);

		VkDescriptorSet Sets[50][2];
		VkDescriptorPool Pool;
		VkDescriptorSetLayout Layout;
		VkDevice Device;

		uint32_t DynamicTexSetsIndex = 0;
	};

	class VulkanRHIDevice : public RHIDevice {
	public:
		VulkanRHIDevice(Window* window, bool useImgui);
		virtual ~VulkanRHIDevice() override;

		virtual RHIData CreateTexture2DRHI(const Texture2DDesc& desc) override;
		virtual void DestroyTexture2DRHI(RHIData data) override;
		virtual void MipMapTexture2D(RHICommandBuffer* cmd, RHITexture2D* tex, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess, uint32_t numMips) override;

		virtual RHIData CreateCubeTextureRHI(const CubeTextureDesc& desc) override;
		virtual void DestroyCubeTextureRHI(RHIData data) override;

		virtual void CopyTexture(RHICommandBuffer* cmd, RHITexture* src, const TextureCopyRegion& srcRegion, RHITexture* dst, 
			const TextureCopyRegion& dstRegion, Vec2Uint copySize) override;
		virtual void CopyFromTextureToCPU(RHICommandBuffer* cmd, RHITexture* src, SubResourceCopyRegion region, RHIBuffer* dst) override;
		virtual void ClearTexture(RHICommandBuffer* cmd, RHITexture* tex, EGPUAccessFlags access, const Vec4& color) override;
		virtual void CopyDataToTexture(void* src, size_t srcOffset, RHITexture* dst, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess, 
			const std::vector<SubResourceCopyRegion>& regions, size_t copySize) override;
		virtual void BarrierTexture(RHICommandBuffer* cmd, RHITexture* texture, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;

		virtual RHIData CreateTextureViewRHI(const TextureViewDesc& desc) override;
		virtual void DestroyTextureViewRHI(RHIData data) override;

		virtual RHIData CreateBufferRHI(const BufferDesc& desc) override;
		virtual void DestroyBufferRHI(RHIData data) override;
		virtual void CopyBuffer(RHICommandBuffer* cmd, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) override;
		virtual void* MapBufferMem(RHIBuffer* buffer) override;
		virtual void BarrierBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;
		virtual void FillBuffer(RHICommandBuffer* cmd, RHIBuffer* buffer, size_t size, size_t offset, uint32_t value, EGPUAccessFlags lastAccess, EGPUAccessFlags newAccess) override;
		virtual uint64_t GetBufferGPUAddress(RHIBuffer* buffer) override;

		virtual RHIData CreateBindingSetLayoutRHI(const BindingSetLayoutDesc& desc) override;
		virtual void DestroyBindingSetLayoutRHI(RHIData data) override;

		virtual RHIData CreateBindingSetRHI(RHIBindingSetLayout* layout) override;
		virtual void DestroyBindingSetRHI(RHIData data) override;

		virtual RHIData CreateShaderRHI(const ShaderDesc& desc, const ShaderCompiler::BinaryShader& binaryShader, const std::vector<RHIBindingSetLayout*>& layouts) override;
		virtual void DestroyShaderRHI(RHIData data) override;
		virtual void BindShader(RHICommandBuffer* cmd, RHIShader* shader, std::vector<RHIBindingSet*> shaderSets = {}, void* pushData = nullptr) override;

		virtual RHIData CreateSamplerRHI(const SamplerDesc& desc) override;
		virtual void DestroySamplerRHI(RHIData data) override;

		virtual RHIData CreateCommandBufferRHI() override;
		virtual void DestroyCommandBufferRHI(RHIData data) override;
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
		virtual void DrawSwapchain(RHICommandBuffer* cmd, uint32_t width, uint32_t height, ImGuiRTState* guiState = nullptr, RHITexture2D* fillTexture = nullptr) override;

	private:
		void UpdateImGuiObjects(ImGuiRTState* state);

	private:
		VulkanDevice m_Device;
		VulkanSwapchain m_Swapchain;

		VulkanImGuiTextureManager m_GuiTextureManager;
		bool m_UsingImGui;

		VkDescriptorPool m_BindlessPool;
		VkDescriptorPool m_GlobalSetPool;

		RHICommandBuffer* m_ImmCmd;
		VkFence m_ImmFence;

		struct {

			VkSemaphore SwapchainSemaphore, RenderSemaphore;
			VkFence RenderFence;
		} m_SyncObjects[2];

		struct {
			VulkanRHIBuffer* VtxBuffer = nullptr;
			VulkanRHIBuffer* IdxBuffer = nullptr;

			size_t VtxBufferSize = 0;
			size_t IdxBufferSize = 0;
		} m_ImGuiDrawObjects[2];

		VulkanRHIShader* m_ImGuiShader;
		VulkanRHIBindingSetLayout* m_ImGuiLayout;
		VulkanRHIBindingSetLayout* m_ImGuiTexLayout;
	};
}
