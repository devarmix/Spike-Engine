#pragma once

#include <Engine/Renderer/GfxDevice.h>
#include <Platforms/Vulkan/VulkanDevice.h>
#include <Platforms/Vulkan/VulkanSwapchain.h>
#include <Platforms/Vulkan/VulkanMaterial.h>
#include <Platforms/Vulkan/VulkanDescriptors.h>
#include <SDL.h>

namespace Spike {

	struct FuncQueue {

		std::deque<std::function<void()>> Queue;

		void PushFunction(std::function<void()>&& function) {
			Queue.push_back(function);
		}

		void ReversedFlush() {

			// reverse iterate the queue to execute all the functions
			for (auto it = Queue.rbegin(); it != Queue.rend(); it++) {
				// call the functions
				(*it)();
			}

			Queue.clear();
		}

		void Flush() {

			// iterate the queue to execute all the functions
			for (auto it = Queue.begin(); it != Queue.end(); it++) {
				// call the functions
				(*it)();
			}

			Queue.clear();
		}
	};

	struct DrawIndirectCommand {

		uint32_t VertexCount;     
		uint32_t InstanceCount;  
		uint32_t FirstVertex; 
		uint32_t FirstInstance; 
	};

	struct alignas(16) SceneObjectMetadata {

		glm::vec4 BoundsOrigin; // w - bounds radius
		glm::vec4 BoundsExtents;
		glm::mat4 GlobalTransform;

		uint32_t FirstIndex;
		uint32_t IndexCount;

		VkDeviceAddress IndexBufferAddress;
		VkDeviceAddress VertexBufferAddress;

		uint32_t MaterialBufferIndex;
		uint32_t DrawBatchID;

		int LastVisibilityIndex;
		int CurrentVisibilityIndex;

		float pad0[2];
	};

	constexpr uint32_t MAX_SCENE_DRAWS_PER_FRAME = 8;
	constexpr uint32_t MAX_DRAWS_PER_FRAME = 100000;
	constexpr uint32_t MAX_LIGHTS_PER_FRAME = 5000;
	constexpr uint32_t MAX_MATERIALS_PER_FRAME = 1000;

	struct FrameData {

		VkCommandPool CommandPool;
		VkCommandBuffer MainCommandBuffer;

		VkSemaphore SwapchainSemaphore, RenderSemaphore;
		VkFence RenderFence;

		FuncQueue ExecutionQueue;
		DescriptorAllocator FrameAllocator;

		VulkanBuffer SceneDataBuffer;
		uint32_t SceneDataBufferOffset;

		VulkanBuffer LightsBuffer;
		uint32_t SceneLightFirstIndex;

		VulkanBuffer DrawCommandsBuffer;
		uint32_t SceneDrawCommandFirstIndex;

		VulkanBuffer DrawCountBuffer;
		uint32_t SceneDrawCountFirstIndex;

		VulkanBuffer SceneObjectMetaBuffer;
		uint32_t SceneObjectFirstIndex;

		VulkanBuffer BatchOffsetsBuffer;
		uint32_t SceneBatchOffsetFirstIndex;

		VulkanBuffer CullDataBuffer;
		uint32_t SceneCullDataFirstIndex;

		VulkanBuffer VisibilityBuffer;
		uint32_t SceneVisibilityBufferFirstIndex;
	};

	struct VulkanPipeline {

		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

	struct DrawBatch {

		uint32_t CommandsOffset;
		uint32_t CommandsCount;

		VulkanMaterialNativeData* MatNativeData;
	};

	constexpr uint32_t FRAME_OVERLAP = 2;

	struct VulkanImGuiTextureManager {

		void Init(uint32_t numDynamicTextures);
		void Cleanup();

		ImTextureID CreateStaticGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler);
		void DestroyStaticGuiTexture(ImTextureID id);

		ImTextureID CreateDynamicGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler, uint32_t currentFrameIndex);

		std::vector<VkDescriptorSet> DynamicTexSets[FRAME_OVERLAP];
		uint32_t DynamicTexSetsIndex = 0;
	};

	class VulkanGfxDevice : public GfxDevice {
	public:

		VulkanGfxDevice(Window* window, bool useImgui);
		virtual ~VulkanGfxDevice() override;

		//virtual void InitDevice(Window* window) override;

		virtual ResourceGPUData* CreateTexture2DGPUData(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData) override;
		virtual void DestroyTexture2DGPUData(ResourceGPUData* data) override;

		virtual ResourceGPUData* CreateCubeTextureGPUData(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture) override;
		virtual ResourceGPUData* CreateFilteredCubeTextureGPUData(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode) override;
		virtual void DestroyCubeTextureGPUData(ResourceGPUData* data) override;

		virtual ResourceGPUData* CreateMaterialGPUData(EMaterialSurfaceType surfaceType) override;
		virtual void DestroyMaterialGPUData(ResourceGPUData* data) override;

		virtual ResourceGPUData* CreateMeshGPUData(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices) override;
		virtual void DestroyMeshGPUData(ResourceGPUData* data) override;

		virtual ResourceGPUData* CreateDepthMapGPUData(uint32_t width, uint32_t height, uint32_t depthPyramidSize) override;
		virtual void DestroyDepthMapGPUData(ResourceGPUData* data) override;

		virtual ResourceGPUData* CreateBloomMapGPUData(uint32_t width, uint32_t height) override;
		virtual void DestroyBloomMapGPUData(ResourceGPUData* data) override;

		virtual void SetMaterialScalarParameter(MaterialResource* material, uint8_t dataBindIndex, float newValue) override;
		virtual void SetMaterialColorParameter(MaterialResource* material, uint8_t dataBindIndex, Vector4 newValue) override;
		virtual void SetMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex, Texture2DResource* newValue) override;
		virtual void AddMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex) override;
		virtual void RemoveMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex) override;

		virtual ImTextureID CreateStaticGuiTexture(Texture2DResource* texture) override;
		virtual void DestroyStaticGuiTexture(ImTextureID id) override;
		virtual ImTextureID CreateDynamicGuiTexture(Texture2DResource* texture) override;

		virtual void DrawSceneIndirect(SceneRenderProxy* scene, RenderContext renderContext) override;
		virtual void DrawSwapchain(uint32_t width, uint32_t height, bool drawGui = false, Texture2DResource* fillTexture = nullptr) override;
		virtual void BeginFrameCommandRecording() override;

	private:

		FrameData& GetCurrentFrame() { return m_Frames[m_FrameNumber % FRAME_OVERLAP]; }
		FrameData& GetLastFrame() { return m_Frames[(m_FrameNumber + FRAME_OVERLAP - 1) % FRAME_OVERLAP]; }

		uint32_t GetDrawBatchID(VulkanMaterialNativeData* material);

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func);

		void CreateVulkanCubeTexture(uint32_t size, VkFormat format, VkImageUsageFlags usageFlags, uint32_t numMips, VulkanCubeTextureNativeData* outTexture);
		void DestroyVulkanCubeTexture(VulkanCubeTextureNativeData texture);

		void CreateVulkanTexture2D(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usageFlags, uint32_t numMips, void* pixelData, VulkanTextureNativeData* outTexture);
		void DestroyVulkanTexture2D(VulkanTextureNativeData texture);

		void InitPipelines();
		void InitCommands();
		void InitSyncStructures();
		void InitDescriptors();
		void InitImGui(SDL_Window* window);
		void InitDefaultData();
		void CreateBRDFLutTexture();

		void InitSkyboxPipeline();
		void InitLightPipeline();
		void InitDepthPyramidPipeline();
		void InitCullPipeline();
		void InitCubeTextureGenPipeline();
		void InitIrradianceGenPipeline();
		void InitRadianceGenPipeline();
		void InitBRDFLutPipeline();
		void InitBloomDownSamplePipeline();
		void InitBloomUpSamplePipeline();
		void InitSSAOPipeline();
		void InitSSAOBlurPipeline();

		void GBufferPass(SceneRenderProxy* scene, RenderContext context, VkDescriptorSet geometrySet);
		void CullPass(SceneRenderProxy* scene, VkDescriptorSet geometrySet, bool prepass);
		void GeometryDrawPass(SceneRenderProxy* scene, RenderContext context, VkDescriptorSet geometrySet, bool prepass);
		void SkyboxPass(RenderContext context, VkDescriptorSet lightingSet);
		void LightingPass(SceneRenderProxy* scene, RenderContext context, VkDescriptorSet lightingSet);
		void BloomPass(RenderContext context);
		void SSAOPass(RenderContext context, VkDescriptorSet geometrySet);

		void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView, bool clearSwapchain = true);

	private:

		VulkanDevice m_Device;
		VulkanSwapchain m_Swapchain;

		VkSampler m_DefSamplerLinearClamped;
		VkSampler m_DefSamplerLinear;
		VkSampler m_DefSamplerNearest;

		FuncQueue m_MainDeletionQueue;
		VulkanMaterialManager m_MaterialManager;
		VulkanImGuiTextureManager m_GuiTextureManager;

		VkFence m_ImmFence;
		VkCommandBuffer m_ImmCommandBuffer;
		VkCommandPool m_ImmCommandPool;

		VulkanPipeline m_LightingPipeline;

		struct alignas(16) LightingData {

			uint32_t SceneDataOffset;
			uint32_t LightsOffset;
			uint32_t LightsCount;
			uint32_t EnvironmentMapNumMips;
		};

		VulkanPipeline m_SkyboxPipeline;

		struct alignas(16) SkyboxData {

			uint32_t SceneDataOffset;
			float pad0[3];
		};

		VulkanPipeline m_CullPipeline;

		struct alignas(16) CullData {

			glm::vec4 FrustumPlanes[6];

			float P00;
			float P11;
			uint32_t PyramidSize;

			float pad0;
		};

		struct alignas(16) CullPushData {

			uint32_t MeshCount;
			uint32_t SceneDataOffset;
			uint32_t DrawCountOffset;
			uint32_t DrawCommandOffset;
			uint32_t MetaDataOffset;
			uint32_t BatchOffset;
			uint32_t CullDataOffset;

			uint32_t IsPrepass;
		};
		
		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
			VkSampler DepthPyramidSampler;
		} m_DepthPyramidPipeline;

		struct alignas(16) DepthReduceData {

			uint32_t PyramidSize;
			float pad0[3];
		};

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
			VkDescriptorSet Set;
		} m_CubeTextureGenPipeline;

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
			VkDescriptorSet Set;
		} m_IrradianceGenPipeline;

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
			VkDescriptorSet Set;
		} m_RadianceGenPipeline;

		struct alignas(16) CubeFilteringData {

			glm::mat4 MVP;

			float Roughness;
			float pad0[3];
		};

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
		} m_BloomDownSamplePipeline;

		struct alignas(16) BloomDownSampleData {

			glm::vec2 SrcSize;
			glm::vec2 OutSize;

			uint32_t MipLevel;
			float Threadshold;
			float SoftThreadshold;

			float pad0;
		};

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
		} m_BloomUpSamplePipeline;

		struct alignas(16) BloomUpSampleData {

			glm::vec2 OutSize;

			uint32_t IsComposite;
			float BloomIntensity;
			float FilterRadius;

			float pad0[3];
		};

		VulkanPipeline m_BRDFLutPipeline;
		VulkanTextureNativeData m_BRDFLutTexture;

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
		} m_SSAOPipeline;

		struct alignas(16) SSAOData {

		    glm::vec2 TexSize;

			float Radius;
			float Bias;

			uint32_t SceneDataOffset;
			uint32_t NumSamples;
			float Intensity;

			float pad0;
		};

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;
			VkDescriptorSetLayout SetLayout;
		} m_SSAOBlurPipeline;

		struct alignas(16) SSAOBlurData {

			glm::vec2 TexSize;
			float pad0[2];
		};

		VulkanTextureNativeData m_SSAONoiseTexture;
		 
		DescriptorAllocator m_DescriptorAllocator;

		VkDescriptorSetLayout m_GeometrySetLayout;
		VkDescriptorSetLayout m_LightingSetLayout;

		FrameData m_Frames[FRAME_OVERLAP];
		uint32_t m_FrameNumber = 0;

		std::vector<DrawBatch> m_DrawBatches;
		std::unordered_map<uint64_t, uint32_t> m_DrawBatchIDMap;
		uint32_t m_DrawBatchIDCounter;
	};
}
