#pragma once

#include <Platforms/Vulkan/VulkanDevice.h>
#include <Platforms/Vulkan/VulkanSwapchain.h>
#include <Platforms/Vulkan/VulkanBuffer.h>
#include <Platforms/Vulkan/VulkanDescriptors.h>
#include <Platforms/Vulkan/VulkanMaterial.h>
#include <Platforms/Vulkan/VulkanTexture.h>
#include <Platforms/Vulkan/VulkanCubeTexture.h>
#include <Platforms/Vulkan/VulkanMesh.h>
#include <SDL.h>

#include <Engine/Renderer/PerspectiveCamera.h>

using namespace SpikeEngine;

namespace Spike {

	struct GPUSceneData {

		glm::mat4 View;
		glm::mat4 Proj;
		glm::mat4 ViewProj;
		glm::mat4 InverseProj;
		glm::mat4 InverseView;

		glm::vec3 CameraPos;
		float AmbientIntensity;
	};

	struct DeletionQueue {

		std::deque<std::function<void()>> Deletors;

		void PushFunction(std::function<void()>&& function) {
			Deletors.push_back(function);
		}

		void Flush() {

			// reverse iterate the deletion queue to execute all the functions
			for (auto it = Deletors.rbegin(); it != Deletors.rend(); it++) {
				// call the functions
				(*it)();
			}

			Deletors.clear();
		}
	};

	struct FrameData {

		VkCommandPool CommandPool;
		VkCommandBuffer MainCommandBuffer;

		VkSemaphore SwapchainSemaphore, RenderSemaphore;
		VkFence RenderFence;

		DeletionQueue DeletionQueue;
		DescriptorAllocatorGrowable FrameDescriptors;
	};

	struct RenderObject {

		uint32_t IndexCount;
		uint32_t FirstIndex;
		VkBuffer IndexBuffer;

		Ref<Material> Material;
		MeshBounds Bounds;

		glm::mat4 Transform;
		VkDeviceAddress VertexBufferAddress;
	};

	struct LightObject {

		glm::vec3 Color;
		float Intensity;

		glm::vec3 Position;
		float InnerAngle;

		glm::vec3 Direction;
		float OuterAngle;

		int Type;
		float Radius;

		int extra[2];
	};

	struct GeometryBuffer {

		Ref<VulkanTexture> AlbedoMap;
		Ref<VulkanTexture> NormalMap;
		Ref<VulkanTexture> DepthMap;
		Ref<VulkanTexture> MaterialMap; // Holds: Metallic map / Roughness map / AO map as RGB values
		//Ref<VulkanTexture> PositionMap;

		void Init(uint32_t width, uint32_t height);
		void Resize(uint32_t width, uint32_t height);
		void Destroy();
	};

	struct DrawContext {

		std::vector<RenderObject> OpaqueSurfaces;
		std::vector<RenderObject> TransparentSurfaces;

		//std::vector<LightObject> Lights;
		uint32_t LightCount = 0;

		VkDescriptorSet FrameSceneDataSet;
	};

	enum VulkanRendererMode : uint8_t {

		RendererModeDefault,
		RendererModeViewport
	};

	constexpr unsigned int FRAME_OVERLAP = 2;

	class VulkanRenderer {
	public:

		static bool IsInitialized;
		static int FrameNumber;

		static VulkanDevice Device;
		static VulkanSwapchain Swapchain;

		static VkDescriptorSetLayout SceneDataDescriptorLayout;

		static FrameData Frames[FRAME_OVERLAP];
		static FrameData& GetCurrentFrame() { return Frames[FrameNumber % FRAME_OVERLAP]; }

		static DeletionQueue MainDeletionQueue;

		static GPUSceneData SceneData;
		static DescriptorAllocatorGrowable GlobalDescriptorAllocator;

		static VkExtent2D WindowExtent;
		static VkExtent2D DrawExtent;
		static VkExtent2D ViewportExtent;

		static float RenderScale;

		static VkFence ImmFence;
		static VkCommandBuffer ImmCommandBuffer;
		static VkCommandPool ImmCommandPool;

		static Ref<VulkanTexture> DefWhiteTexture;
		static Ref<VulkanTexture> DefErrorTexture;
		static Ref<VulkanCubeTexture> DefErrorCubeTexture;

		static Ref<VulkanTexture> DrawTexture;
		static Ref<VulkanTexture> ViewportTexture;

		static VkSampler DefSamplerLinear;
		static VkSampler DefSamplerNearest;

		static DrawContext MainDrawContext;

		static PerspectiveCamera* MainCamera;

		static bool ViewportMinimized;
		static VulkanRendererMode RendererMode;

		static VulkanMaterialManager GlobalMaterialManager;

		static GeometryBuffer GBuffer;

		static struct BRDFLutPipelineData {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;

			VkCommandPool CommandPool;
			VkCommandBuffer Commandbuffer;
			VkFence GenFence;

			static constexpr uint32_t TextureSize = 512;

			// brdf lookup texture
			Ref<VulkanTexture> BRDFLutTexture;

		} BRDFLutPipeline;

		static struct LightingPipelineData {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;

			VkDescriptorSet Set;
			VkDescriptorSetLayout SetLayout;

			// stores all the lights for this frame
			VulkanBuffer LightBuffer;

		} LightingPipeline;

		static struct SkyboxPipelineData {

			VkPipeline Pipeline;
			VkPipelineLayout PipelineLayout;

			VkDescriptorSet Set;
			VkDescriptorSetLayout SetLayout;

			// hdr skybox texture
			Ref<VulkanCubeTexture> SkyboxTexture;

		} SkyboxPipeline;

	public:

		static void Init(Window& window, VulkanRendererMode mode = RendererModeDefault);
		static void Cleanup();
		static void Draw();

		static void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		static void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& worldTransform);
		static void SubmitLight(const float raduis, const float intensity, const glm::vec4& color, const glm::vec3& position);

		static void ResizeSwapchain(uint32_t width, uint32_t height);
		static void ResizeViewport(uint32_t width, uint32_t height);

		static void PoolImGuiEvents(SDL_Event& event);

	private:

		static void InitSwapchain(uint32_t width, uint32_t height);
		static void InitPipelines();
		static void InitCommands();
		static void InitSyncStructures();
		static void InitDescriptors();
		static void InitImGui(SDL_Window* window);
		static void InitDefaultData();

		static void InitBRDFLutPipeline();
		static void InitSkyboxPipeline();
		static void InitLightPipeline();

		static void InitViewport(uint32_t width, uint32_t height);
		static void DestroyViewport();

		static bool BeginFrame(VkCommandBuffer cmd, uint32_t* swapchainImageIndex);
		static void EndFrame(VkCommandBuffer cmd, uint32_t swapchainImageIndex);

		static void GenerateBRDFLut();

		static void DrawBackground(VkCommandBuffer cmd);
		static void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView);
		static void DrawDefault(VkCommandBuffer cmd, uint32_t swapchainImageIndex);
		static void DrawViewport(VkCommandBuffer cmd, uint32_t swapchainImageIndex);
		static void DrawGeometry(VkCommandBuffer cmd);

		static void SkyboxPass(VkCommandBuffer cmd);
		static void BRDFLutPass(VkCommandBuffer cmd);
		static void GBufferPass(VkCommandBuffer cmd);
		static void SSAOPass();
		static void LightingPass(VkCommandBuffer cmd);
		static void TransparentPass();

		static void UpdateScene();
	};
}
