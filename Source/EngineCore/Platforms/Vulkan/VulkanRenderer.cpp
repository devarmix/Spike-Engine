#include <Platforms/Vulkan/VulkanRenderer.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <Platforms/Vulkan/VulkanTools.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <Engine/Core/Stats.h>
#include <Platforms/Vulkan/VulkanPipeline.h>

#include <Engine/Core/Profiler.h>

namespace Spike {

	constexpr bool bUseValidationLayers = true;

	bool VulkanRenderer::IsInitialized = false;
	int VulkanRenderer::FrameNumber = 0;

	VulkanDevice VulkanRenderer::Device;
	VulkanSwapchain VulkanRenderer::Swapchain;

	VkDescriptorSetLayout VulkanRenderer::SceneDataDescriptorLayout;

	FrameData VulkanRenderer::Frames[FRAME_OVERLAP];

	DeletionQueue VulkanRenderer::MainDeletionQueue;

	GPUSceneData VulkanRenderer::SceneData;
    DescriptorAllocatorGrowable VulkanRenderer::GlobalDescriptorAllocator;

	VkExtent2D VulkanRenderer::WindowExtent{ 1920, 1080 };
	VkExtent2D VulkanRenderer::DrawExtent;
	VkExtent2D VulkanRenderer::ViewportExtent;

	float VulkanRenderer::RenderScale = 1.f;

	VkFence VulkanRenderer::ImmFence;
	VkCommandBuffer VulkanRenderer::ImmCommandBuffer;
	VkCommandPool VulkanRenderer::ImmCommandPool;

	Ref<VulkanTexture> VulkanRenderer::DefWhiteTexture;
	Ref<VulkanTexture> VulkanRenderer::DefErrorTexture;
	Ref<VulkanCubeTexture> VulkanRenderer::DefErrorCubeTexture;

	Ref<VulkanTexture> VulkanRenderer::DrawTexture;
	Ref<VulkanTexture> VulkanRenderer::ViewportTexture;

	VkSampler VulkanRenderer::DefSamplerLinear;
	VkSampler VulkanRenderer::DefSamplerNearest;

    DrawContext VulkanRenderer::MainDrawContext;

	PerspectiveCamera* VulkanRenderer::MainCamera = nullptr;

	bool VulkanRenderer::ViewportMinimized = false;
	VulkanRendererMode VulkanRenderer::RendererMode = RendererModeDefault;

	VulkanMaterialManager VulkanRenderer::GlobalMaterialManager;

	GeometryBuffer VulkanRenderer::GBuffer;

	VulkanRenderer::BRDFLutPipelineData VulkanRenderer::BRDFLutPipeline;
	VulkanRenderer::LightingPipelineData VulkanRenderer::LightingPipeline;
	VulkanRenderer::SkyboxPipelineData VulkanRenderer::SkyboxPipeline;

	static bool IsVisible(const RenderObject& obj, const glm::mat4& viewproj) {

		std::array<glm::vec3, 8> corners{
			glm::vec3 { 1, 1, 1 },
			glm::vec3 { 1, 1, -1 },
			glm::vec3 { 1, -1, 1 },
			glm::vec3 { 1, -1, -1 },
			glm::vec3 { -1, 1, 1 },
			glm::vec3 { -1, 1, -1 },
			glm::vec3 { -1, -1, 1 },
			glm::vec3 { -1, -1, -1 },
		};

		glm::mat4 matrix = viewproj * obj.Transform;

		glm::vec3 min = { 1.5, 1.5, 1.5 };
		glm::vec3 max = { -1.5, -1.5, -1.5 };

		for (int c = 0; c < 8; c++) {
			// project each corner into clip space

			glm::vec3 bOrigin = glm::vec3(obj.Bounds.Origin.x, obj.Bounds.Origin.y, obj.Bounds.Origin.z);
			glm::vec3 bExtents = glm::vec3(obj.Bounds.Extents.x, obj.Bounds.Extents.y, obj.Bounds.Extents.z);

			glm::vec4 v = matrix * glm::vec4(bOrigin + (corners[c] * bExtents), 1.f);

			// perspective correction
			v.x = v.x / v.w;
			v.y = v.y / v.w;
			v.z = v.z / v.w;

			min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
			max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
		}

		// check the clip space box is within the view
		if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
			return false;
		}
		else {
			return true;
		}
	}


	void VulkanRenderer::Init(Window& window, VulkanRendererMode mode) {

		assert(!IsInitialized);

		Device.Init(window, bUseValidationLayers);

		RendererMode = mode;
		if (mode == RendererModeViewport)
			InitViewport(window.GetWidth(), window.GetHeight());

		InitSwapchain(window.GetWidth(), window.GetHeight());
		InitCommands();
		InitSyncStructures();
		InitDescriptors();
		InitImGui((SDL_Window*)window.GetNativeWindow());
		InitDefaultData();

		InitPipelines();
		GenerateBRDFLut();

		GlobalMaterialManager.Init(1000);

		IsInitialized = true;
	}

	void VulkanRenderer::Cleanup() {

		assert(IsInitialized);

		vkDeviceWaitIdle(Device.Device);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			vkDestroyCommandPool(Device.Device, Frames[i].CommandPool, nullptr);

			// destroy sync objects
			vkDestroyFence(Device.Device, Frames[i].RenderFence, nullptr);
			vkDestroySemaphore(Device.Device, Frames[i].RenderSemaphore, nullptr);
			vkDestroySemaphore(Device.Device, Frames[i].SwapchainSemaphore, nullptr);

			Frames[i].DeletionQueue.Flush();
		}

		MainDeletionQueue.Flush();

		if (RendererMode == RendererModeViewport)
			DestroyViewport();

		GlobalMaterialManager.Cleanup();

		Swapchain.Destroy();
		Device.Destroy();

		IsInitialized = false;
	}

	void VulkanRenderer::InitPipelines() {

		InitBRDFLutPipeline();
		InitSkyboxPipeline();
		InitLightPipeline();
	}

	void VulkanRenderer::InitBRDFLutPipeline() {

		// create brdf lut texture
		{
			VkImageUsageFlags textureFlags{};
			textureFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			textureFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			VkExtent3D textureSize = {

				BRDFLutPipelineData::TextureSize,
				BRDFLutPipelineData::TextureSize,
				1
			};

			BRDFLutPipeline.BRDFLutTexture = VulkanTexture::Create(textureSize, VK_FORMAT_R16G16_SFLOAT, textureFlags);
		}

		// init pipeline
		{
			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 0;
			layout_Info.pSetLayouts = VK_NULL_HANDLE;
			layout_Info.pPushConstantRanges = VK_NULL_HANDLE;
			layout_Info.pushConstantRangeCount = 0;

			VkPipelineLayout newLayout;
			VK_CHECK(vkCreatePipelineLayout(Device.Device, &layout_Info, nullptr, &newLayout));

			BRDFLutPipeline.PipelineLayout = newLayout;

			VulkanShader shader = VulkanShader::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/BRDFLut.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/BRDFLut.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			pipelineBulder.SetColorAttachments(&BRDFLutPipeline.BRDFLutTexture->GetRawData()->Format, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = newLayout;
			pipelineBulder.DisableBlending();
			pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			BRDFLutPipeline.Pipeline = pipelineBulder.BuildPipeline(Device.Device);

			shader.Destroy();
		}

		// init command / sync structures
		{
			VkCommandPoolCreateInfo commandPoolInfo = VulkanTools::CommandPoolCreateInfo(Device.Queues.GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			VK_CHECK(vkCreateCommandPool(Device.Device, &commandPoolInfo, nullptr, &BRDFLutPipeline.CommandPool));

			VkCommandBufferAllocateInfo cmdAllocInfo = VulkanTools::CommandBufferAllocInfo(BRDFLutPipeline.CommandPool, 1);
			VK_CHECK(vkAllocateCommandBuffers(Device.Device, &cmdAllocInfo, &BRDFLutPipeline.Commandbuffer));

			VkFenceCreateInfo fenceCreateInfo = VulkanTools::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
			VK_CHECK(vkCreateFence(Device.Device, &fenceCreateInfo, nullptr, &BRDFLutPipeline.GenFence));
		}

		MainDeletionQueue.PushFunction([=]() {

			vkDestroyPipelineLayout(Device.Device, BRDFLutPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(Device.Device, BRDFLutPipeline.Pipeline, nullptr);

			BRDFLutPipeline.BRDFLutTexture->Destroy();

			vkDestroyCommandPool(Device.Device, BRDFLutPipeline.CommandPool, nullptr);
			vkDestroyFence(Device.Device, BRDFLutPipeline.GenFence, nullptr);
		});
	}

	void VulkanRenderer::InitSkyboxPipeline() {

		// init descriptor set layout
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // skybox image

			SkyboxPipeline.SetLayout = builder.Build(Device.Device, VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		// init pipeline
		{
			VkDescriptorSetLayout layouts[] = { SceneDataDescriptorLayout, SkyboxPipeline.SetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 2;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = VK_NULL_HANDLE;
			layout_Info.pushConstantRangeCount = 0;

			VkPipelineLayout newLayout;
			VK_CHECK(vkCreatePipelineLayout(Device.Device, &layout_Info, nullptr, &newLayout));

			SkyboxPipeline.PipelineLayout = newLayout;

			VulkanShader shader = VulkanShader::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/Skybox.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/Skybox.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			pipelineBulder.SetColorAttachments(&DrawTexture->GetRawData()->Format, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = newLayout;
			pipelineBulder.DisableBlending();
			pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			SkyboxPipeline.Pipeline = pipelineBulder.BuildPipeline(Device.Device);

			shader.Destroy();
		}

		// init descriptor set
		SkyboxPipeline.Set = GlobalDescriptorAllocator.Allocate(Device.Device, SkyboxPipeline.SetLayout);

		// load skybox texture
		{
			std::array<const char*, 6> filePath = {

				"C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/FluffballDay/output_skybox_posx.hdr",
				"C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/FluffballDay/output_skybox_negx.hdr",
				"C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/FluffballDay/output_skybox_posy.hdr",
				"C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/FluffballDay/output_skybox_negy.hdr",
				"C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/FluffballDay/output_skybox_posz.hdr",
				"C:/Users/Artem/Desktop/Spike-Engine/Resources/Test/textures/HDR/FluffballDay/output_skybox_negz.hdr"
			};

			SkyboxPipeline.SkyboxTexture = VulkanCubeTexture::Create(filePath);
		}

		// write set
		{
			DescriptorWriter writer;
			writer.WriteImage(0, SkyboxPipeline.SkyboxTexture->GetRawData()->View, DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.UpdateSet(Device.Device, SkyboxPipeline.Set);
		}

		MainDeletionQueue.PushFunction([=]() {

			vkDestroyDescriptorSetLayout(Device.Device, SkyboxPipeline.SetLayout, nullptr);
			vkDestroyPipelineLayout(Device.Device, SkyboxPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(Device.Device, SkyboxPipeline.Pipeline, nullptr);

			SkyboxPipeline.SkyboxTexture->Destroy();
		});
	}

	void VulkanRenderer::InitLightPipeline() {

		// init descriptor set layout
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // light buffer
			builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // albedo image
			builder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // normal image
			builder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // material image
			builder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // depth image
			builder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // skybox image

			LightingPipeline.SetLayout = builder.Build(Device.Device, VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		// init pipeline
		{
			VkDescriptorSetLayout layouts[] = { SceneDataDescriptorLayout, LightingPipeline.SetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 2;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = VK_NULL_HANDLE;
			layout_Info.pushConstantRangeCount = 0;

			VkPipelineLayout newLayout;
			VK_CHECK(vkCreatePipelineLayout(Device.Device, &layout_Info, nullptr, &newLayout));

			LightingPipeline.PipelineLayout = newLayout;

			VulkanShader shader = VulkanShader::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/PBRLightingTest.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/PBRLightingTest.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			pipelineBulder.SetColorAttachments(&DrawTexture->GetRawData()->Format, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = newLayout;
			pipelineBulder.DisableBlending();
			pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			LightingPipeline.Pipeline = pipelineBulder.BuildPipeline(Device.Device);

			shader.Destroy();
		}

		// init descriptor set
		LightingPipeline.Set = GlobalDescriptorAllocator.Allocate(Device.Device, LightingPipeline.SetLayout);

		// update set
		{
			DescriptorWriter writer;
			writer.WriteImage(1, GBuffer.AlbedoMap->GetRawData()->View, DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(2, GBuffer.NormalMap->GetRawData()->View, DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(3, GBuffer.MaterialMap->GetRawData()->View, DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(4, GBuffer.DepthMap->GetRawData()->View, DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(5, SkyboxPipeline.SkyboxTexture->GetRawData()->View, DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.UpdateSet(Device.Device, LightingPipeline.Set);
		}

		// init buffer
		LightingPipeline.LightBuffer = VulkanBuffer::Create(sizeof(LightObject) * 1000, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		{
			LightObject light{};

			light.Radius = 20;
			light.Intensity = 100;
			light.Color = { 1.0, 0.7, 0.2 };
			light.Position = { 3.f, 3.f, 3.f };
			light.Type = 0;

			LightObject* lights = (LightObject*)LightingPipeline.LightBuffer.AllocationInfo.pMappedData;
			lights[0] = light;
		}

		{
			LightObject light{};

			light.Radius = 20;
			light.Intensity = 150;
			light.Color = { 0.3, 0.2, 1.0 };
			light.Position = { 5.f, 4.f, 4.f };
			light.Type = 0;

			LightObject* lights = (LightObject*)LightingPipeline.LightBuffer.AllocationInfo.pMappedData;
			lights[1] = light;
		}

		// update descriptor set with a buffer of all frame lights
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, LightingPipeline.LightBuffer.Buffer, sizeof(LightObject) * 2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

			writer.UpdateSet(Device.Device, LightingPipeline.Set);
		}

		MainDeletionQueue.PushFunction([=]() {

			vkDestroyDescriptorSetLayout(Device.Device, LightingPipeline.SetLayout, nullptr);
			vkDestroyPipelineLayout(Device.Device, LightingPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(Device.Device, LightingPipeline.Pipeline, nullptr);

			LightingPipeline.LightBuffer.Destroy();
		});
	}

	void VulkanRenderer::InitSwapchain(uint32_t width, uint32_t height) {

		WindowExtent.width = width;
		WindowExtent.height = height;

		Swapchain.Init(width, height);
		GBuffer.Init(width, height);

		// create draw image
		VkExtent3D drawImageExtent = {
			2560,
			1440,
			1
		};

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		DrawTexture = VulkanTexture::Create(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);

		MainDeletionQueue.PushFunction([=]() {

			DrawTexture->Destroy();
			GBuffer.Destroy();
		});
	}

	void VulkanRenderer::ResizeSwapchain(uint32_t width, uint32_t height) {

		vkDeviceWaitIdle(Device.Device);

		WindowExtent.width = width;
		WindowExtent.height = height;

		Swapchain.Resize(width, height);

		if (RendererMode == RendererModeDefault)
		    GBuffer.Resize(width, height);
	}

	void GeometryBuffer::Init(uint32_t width, uint32_t height) {

		VkImageUsageFlags textureFlags{};
		textureFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		textureFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		textureFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkExtent3D textureSize = {

			width,
			height,
			1
		};

		AlbedoMap = VulkanTexture::Create(textureSize, VK_FORMAT_R8G8B8A8_UNORM, textureFlags);
		NormalMap = VulkanTexture::Create(textureSize, VK_FORMAT_R16G16B16A16_SFLOAT, textureFlags);
		MaterialMap = VulkanTexture::Create(textureSize, VK_FORMAT_R8G8B8A8_UNORM, textureFlags);
		//PositionMap = VulkanTexture::Create(textureSize, VK_FORMAT_R16G16B16A16_SFLOAT, textureFlags);

		VkImageUsageFlags depthFlags{};
		depthFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		DepthMap = VulkanTexture::Create(textureSize, VK_FORMAT_D32_SFLOAT, depthFlags);
	}

	void GeometryBuffer::Resize(uint32_t width, uint32_t height) {

		vkDeviceWaitIdle(VulkanRenderer::Device.Device);

		Destroy();
		Init(width, height);

		// update lighting set, after invalidating textures
		{
			DescriptorWriter writer;
			writer.WriteImage(1, VulkanRenderer::GBuffer.AlbedoMap->GetRawData()->View, VulkanRenderer::DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(2, VulkanRenderer::GBuffer.NormalMap->GetRawData()->View, VulkanRenderer::DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			//writer.WriteImage(3, VulkanRenderer::GBuffer.PositionMap->GetRawData()->View, VulkanRenderer::DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(3, VulkanRenderer::GBuffer.MaterialMap->GetRawData()->View, VulkanRenderer::DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(4, VulkanRenderer::GBuffer.DepthMap->GetRawData()->View, VulkanRenderer::DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.UpdateSet(VulkanRenderer::Device.Device, VulkanRenderer::LightingPipeline.Set);
		}
	}

	void GeometryBuffer::Destroy() {

		AlbedoMap->Destroy();
		NormalMap->Destroy();
		MaterialMap->Destroy();
		DepthMap->Destroy();
	}

	void VulkanRenderer::InitViewport(uint32_t width, uint32_t height) {

		VkExtent3D extent {

			width,
			height,
			1
		};

		ViewportExtent.width = width;
		ViewportExtent.height = height;

		VkImageUsageFlags flags{};
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		flags |= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		ViewportTexture = VulkanTexture::Create(extent, VK_FORMAT_R16G16B16A16_SFLOAT, flags);
	}

	void VulkanRenderer::ResizeViewport(uint32_t width, uint32_t height) {

		assert(RendererMode == RendererModeViewport);

		vkDeviceWaitIdle(Device.Device);

		DestroyViewport();
		InitViewport(width, height);

		GBuffer.Resize(width, height);
	}

	void VulkanRenderer::DestroyViewport() {

		ViewportTexture->Destroy();
	}

	void VulkanRenderer::InitCommands() {

		// create a command pool for commands submitted to the graphics queue
		VkCommandPoolCreateInfo commandPoolInfo = VulkanTools::CommandPoolCreateInfo(Device.Queues.GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			VK_CHECK(vkCreateCommandPool(Device.Device, &commandPoolInfo, nullptr, &Frames[i].CommandPool));

			// allocate the default command buffer thta we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = VulkanTools::CommandBufferAllocInfo(Frames[i].CommandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(Device.Device, &cmdAllocInfo, &Frames[i].MainCommandBuffer));
		}

		// immediate submit 
		VK_CHECK(vkCreateCommandPool(Device.Device, &commandPoolInfo, nullptr, &ImmCommandPool));

		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanTools::CommandBufferAllocInfo(ImmCommandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(Device.Device, &cmdAllocInfo, &ImmCommandBuffer));

		MainDeletionQueue.PushFunction([=]() {

			vkDestroyCommandPool(Device.Device, ImmCommandPool, nullptr);
		});
	}

	void VulkanRenderer::InitSyncStructures() {

		VkFenceCreateInfo fenceCreateInfo = VulkanTools::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanTools::SemaphoreCreateInfo();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(Device.Device, &fenceCreateInfo, nullptr, &Frames[i].RenderFence));

			VK_CHECK(vkCreateSemaphore(Device.Device, &semaphoreCreateInfo, nullptr, &Frames[i].SwapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(Device.Device, &semaphoreCreateInfo, nullptr, &Frames[i].RenderSemaphore));
		}

		// immediate fence
		VK_CHECK(vkCreateFence(Device.Device, &fenceCreateInfo, nullptr, &ImmFence));

		MainDeletionQueue.PushFunction([=]() {

			vkDestroyFence(Device.Device,ImmFence, nullptr);
		});
	}

	void VulkanRenderer::InitDescriptors() {

		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {

			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5}
		};

		GlobalDescriptorAllocator.Init(Device.Device, 10, sizes);

		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			SceneDataDescriptorLayout = builder.Build(Device.Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		MainDeletionQueue.PushFunction([&]() {

			GlobalDescriptorAllocator.DestroyPools(Device.Device);
			vkDestroyDescriptorSetLayout(Device.Device, SceneDataDescriptorLayout, nullptr);
		});

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			// create a descriptor pool
			std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_Sizes = {
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 }
			};

			Frames[i].FrameDescriptors = DescriptorAllocatorGrowable{};
			Frames[i].FrameDescriptors.Init(Device.Device, 1000, frame_Sizes);

			MainDeletionQueue.PushFunction([&, i]() {

				Frames[i].FrameDescriptors.DestroyPools(Device.Device);
			});
		}
	}

	void VulkanRenderer::InitDefaultData() {

		uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
		DefWhiteTexture = VulkanTexture::Create((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);

		uint32_t pink = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		DefErrorTexture = VulkanTexture::Create((void*)&pink, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);
		{
			std::array<void*, 6> cubeData = {

				(void*)&pink,
				(void*)&pink,
				(void*)&pink,
				(void*)&pink,
				(void*)&pink,
				(void*)&pink
			};

			DefErrorCubeTexture = VulkanCubeTexture::Create(cubeData, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_USAGE_SAMPLED_BIT);
		}

		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		sampl.magFilter = VK_FILTER_NEAREST;
		sampl.minFilter = VK_FILTER_NEAREST;

		vkCreateSampler(Device.Device, &sampl, nullptr, &DefSamplerNearest);

		sampl.magFilter = VK_FILTER_LINEAR;
		sampl.minFilter = VK_FILTER_LINEAR;

		sampl.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampl.maxLod = 50;

		vkCreateSampler(Device.Device, &sampl, nullptr, &DefSamplerLinear);

		MainDeletionQueue.PushFunction([&]() {

			vkDestroySampler(Device.Device, DefSamplerNearest, nullptr);
			vkDestroySampler(Device.Device, DefSamplerLinear, nullptr);

			DefWhiteTexture->Destroy();
			DefErrorTexture->Destroy();
			DefErrorCubeTexture->Destroy();
		});
	}

	void VulkanRenderer::InitImGui(SDL_Window* window) {

		VkDescriptorPoolSize pool_Sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

		VkDescriptorPoolCreateInfo pool_Info = {};
		pool_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_Info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_Info.maxSets = 1000;
		pool_Info.poolSizeCount = (uint32_t)std::size(pool_Sizes);
		pool_Info.pPoolSizes = pool_Sizes;

		VkDescriptorPool imguiPool;
		VK_CHECK(vkCreateDescriptorPool(Device.Device, &pool_Info, nullptr, &imguiPool));

		ImGui::CreateContext();

		ImGui_ImplSDL2_InitForVulkan(window);

		ImGui_ImplVulkan_InitInfo init_Info = {};
		init_Info.Instance = Device.Instance;
		init_Info.PhysicalDevice = Device.PhysicalDevice;
		init_Info.Device = Device.Device;
		init_Info.Queue = Device.Queues.GraphicsQueue;
		init_Info.DescriptorPool = imguiPool;
		init_Info.MinImageCount = 3;
		init_Info.ImageCount = 3;
		init_Info.UseDynamicRendering = true;

		init_Info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		init_Info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_Info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &Swapchain.Format;

		init_Info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_Info);

		ImGui_ImplVulkan_CreateFontsTexture();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;


		MainDeletionQueue.PushFunction([=]() {

			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(Device.Device, imguiPool, nullptr);
		});
	}

	void VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {

		VK_CHECK(vkResetFences(Device.Device, 1, &ImmFence));
		VK_CHECK(vkResetCommandBuffer(ImmCommandBuffer, 0));

		VkCommandBuffer cmd = ImmCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanTools::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdInfo = VulkanTools::CommandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = VulkanTools::SubmitInfo(&cmdInfo, nullptr, nullptr);

		VK_CHECK(vkQueueSubmit2(Device.Queues.GraphicsQueue, 1, &submit, ImmFence));

		VK_CHECK(vkWaitForFences(Device.Device, 1, &ImmFence, true, 9999999999));
	}

	void VulkanRenderer::DrawBackground(VkCommandBuffer cmd) {

		VkClearColorValue clearValue;
		clearValue = { { 0.09f, 0.09f, 0.09f, 1.0f} };

		VkImageSubresourceRange clearRange = VulkanTools::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

		// clear image
		vkCmdClearColorImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
	}

	void VulkanRenderer::DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView) {

		VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(Swapchain.Extent, &colorAttachment, 1, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	void VulkanRenderer::PoolImGuiEvents(const SDL_Event& event) {

		ImGui_ImplSDL2_ProcessEvent(&event);
	}

	void VulkanRenderer::UpdateScene() {

		SceneData.View = MainCamera->GetViewMatrix();
		SceneData.CameraPos = MainCamera->GetPosition();

		if (RendererMode == RendererModeDefault)
			SceneData.Proj = MainCamera->GetProjectionMatrix((float)WindowExtent.width / (float)WindowExtent.height);
		else if (RendererMode == RendererModeViewport)
			SceneData.Proj = MainCamera->GetProjectionMatrix((float)ViewportExtent.width / (float)ViewportExtent.height);

		SceneData.Proj[1][1] *= -1;
		SceneData.ViewProj = SceneData.Proj * SceneData.View;

		SceneData.InverseProj = glm::inverse(SceneData.Proj);
		SceneData.InverseView = glm::inverse(SceneData.View);

		SceneData.AmbientIntensity = 0.1f;
	}

	void VulkanRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& worldTransform) {

		// We do not draw meshes, when viewport is minimized. So prevent from submiting those meshes to renderer. Or there is no camera.
		if (!MainCamera) return;
		if (RendererMode == RendererModeViewport && ViewportMinimized) return;


		glm::mat4 nodeMatrix = glm::mat4{ 1.f };

		for (auto& s : mesh->SubMeshes) {

			VulkanMeshData* meshData = (VulkanMeshData*)mesh->GetData();

			RenderObject def;
			def.IndexCount = s.VertexCount;
			def.FirstIndex = s.StartVertexIndex;
			def.IndexBuffer = meshData->IndexBuffer.Buffer;
			def.Material = s.Material;
			def.Bounds = s.Bounds;

			def.Transform = nodeMatrix;
			def.VertexBufferAddress = meshData->VertexBufferAddress;

			MainDrawContext.OpaqueSurfaces.push_back(def);
		}
	}

	void VulkanRenderer::SubmitLight(const float raduis, const float intensity, const glm::vec4& color, const glm::vec3& position) {

		// We do not draw meshes, when viewport is minimized. So prevent from submiting those meshes to renderer. Or there is no camera.
		if (!MainCamera) return;
		if (RendererMode == RendererModeViewport && ViewportMinimized) return;

		LightObject light{};

		light.Radius = raduis;
		light.Intensity = intensity;
		light.Color = color;
		light.Position = position;

		LightObject* lights = (LightObject*)LightingPipeline.LightBuffer.AllocationInfo.pMappedData;
		lights[MainDrawContext.LightCount] = light;

		MainDrawContext.LightCount++;
	}

	bool VulkanRenderer::BeginFrame(VkCommandBuffer cmd, uint32_t* swapchainImageIndex) {

		VK_CHECK(vkWaitForFences(Device.Device, 1, &GetCurrentFrame().RenderFence, true, 1000000000));

		GetCurrentFrame().DeletionQueue.Flush();
		GetCurrentFrame().FrameDescriptors.ClearPools(Device.Device);

		VK_CHECK(vkResetFences(Device.Device, 1, &GetCurrentFrame().RenderFence));

		VkResult check = vkAcquireNextImageKHR(Device.Device, Swapchain.Swapchain, 1000000000, GetCurrentFrame().SwapchainSemaphore, nullptr, swapchainImageIndex);
		if (check == VK_ERROR_OUT_OF_DATE_KHR) {

			return false;
		}

		// reset command buffer
		VK_CHECK(vkResetCommandBuffer(cmd, 0));

		// begin command buffer recording
		VkCommandBufferBeginInfo cmdBeginInfo = VulkanTools::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// create scene data buffer
	    VulkanBuffer sceneDatabuffer = VulkanBuffer::Create(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// write current scene data
		{
			void* uData;
			vmaMapMemory(Device.Allocator, sceneDatabuffer.Allocation, &uData);

			GPUSceneData* sceneUniformData = (GPUSceneData*)uData;
			*sceneUniformData = SceneData;

			vmaUnmapMemory(Device.Allocator, sceneDatabuffer.Allocation);
		}

		MainDrawContext.FrameSceneDataSet = GetCurrentFrame().FrameDescriptors.Allocate(Device.Device, SceneDataDescriptorLayout);
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, sceneDatabuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

			writer.UpdateSet(Device.Device, MainDrawContext.FrameSceneDataSet);
		}

		GetCurrentFrame().DeletionQueue.PushFunction([=]() mutable {

			sceneDatabuffer.Destroy();
		});

		return true;
	}

	void VulkanRenderer::EndFrame(VkCommandBuffer cmd, uint32_t swapchainImageIndex) {

		// end command buffer recording
		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdInfo = VulkanTools::CommandBufferSubmitInfo(cmd);

		VkSemaphoreSubmitInfo waitInfo = VulkanTools::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().SwapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = VulkanTools::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().RenderSemaphore);

		VkSubmitInfo2 submit = VulkanTools::SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

		VK_CHECK(vkQueueSubmit2(Device.Queues.GraphicsQueue, 1, &submit, GetCurrentFrame().RenderFence));

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &Swapchain.Swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &GetCurrentFrame().RenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		vkQueuePresentKHR(Device.Queues.GraphicsQueue, &presentInfo);

		FrameNumber++;
	}

	void VulkanRenderer::DrawDefault(VkCommandBuffer cmd, uint32_t swapchainImageIndex) {

		// DRAW TO THE MAIN WINDOW

		DrawExtent.height = uint32_t(std::min(Swapchain.Extent.height, DrawTexture->GetRawData()->Extent.height) * RenderScale);
		DrawExtent.width = uint32_t(std::min(Swapchain.Extent.width, DrawTexture->GetRawData()->Extent.width) * RenderScale);

		VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(cmd);

		// prevent from drawing, if there is no camera
		if (MainCamera) {

			VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			//VulkanTexture::TransitionImage(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			DrawGeometry(cmd);

			VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		}
		else {

			VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		}

		VulkanTexture::TransitionImage(cmd, Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VulkanTexture::CopyImageToImage(cmd, DrawTexture->GetRawData()->Image, Swapchain.Images[swapchainImageIndex], DrawExtent, Swapchain.Extent);
	}

	void VulkanRenderer::DrawViewport(VkCommandBuffer cmd, uint32_t swapchainImageIndex) {

		// DRAW TO THE VIEWPORT

		DrawExtent.height = uint32_t(std::min(ViewportExtent.height, DrawTexture->GetRawData()->Extent.height) * RenderScale);
		DrawExtent.width = uint32_t(std::min(ViewportExtent.width, DrawTexture->GetRawData()->Extent.width) * RenderScale);

		VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(cmd);

		VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VulkanTexture::CopyImageToImage(cmd, DrawTexture->GetRawData()->Image, Swapchain.Images[swapchainImageIndex], DrawExtent, Swapchain.Extent);

		// prevent from drawing, if there is no camera or viewport is minimized
		if (MainCamera && !ViewportMinimized) {

			VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			//VkUtil::Transition_Image(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			DrawGeometry(cmd);

			VulkanTexture::TransitionImage(cmd, DrawTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			VulkanTexture::TransitionImage(cmd, ViewportTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VulkanTexture::CopyImageToImage(cmd, DrawTexture->GetRawData()->Image, ViewportTexture->GetRawData()->Image, DrawExtent, ViewportExtent);

			VulkanTexture::TransitionImage(cmd, ViewportTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else {

			// reset counters
			Stats::Data.DrawcallCount = 0;
			Stats::Data.TriangleCount = 0;
			Stats::Data.RenderTime = 0;

			//mainDrawContext.OpaqueSurfaces.clear();
			//mainDrawContext.TransparentSurfaces.clear();

			VulkanTexture::TransitionImage(cmd, ViewportTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	void VulkanRenderer::DrawGeometry(VkCommandBuffer cmd) {

		SkyboxPass(cmd);

		// prepare GBuffer for rendering
		VulkanTexture::TransitionImage(cmd, GBuffer.AlbedoMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.NormalMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.MaterialMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		//VulkanTexture::TransitionImage(cmd, GBuffer.PositionMap->GetRawData()->Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		{
			VkClearColorValue clearValue;
			clearValue = { { 0.0f, 0.0f, 0.0f, 0.0f} };

			VkImageSubresourceRange clearRange = VulkanTools::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

			// clear image
			vkCmdClearColorImage(cmd, GBuffer.AlbedoMap->GetRawData()->Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
		}

		{
			VkClearColorValue clearValue;
			clearValue = { { 0.0f, 0.0f, 0.0f, 0.0f} };

			VkImageSubresourceRange clearRange = VulkanTools::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

			// clear image
			vkCmdClearColorImage(cmd, GBuffer.NormalMap->GetRawData()->Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
		}

		{
			VkClearColorValue clearValue;
			clearValue = { { 0.0f, 0.0f, 0.0f, 0.0f} };

			VkImageSubresourceRange clearRange = VulkanTools::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

			// clear image
			vkCmdClearColorImage(cmd, GBuffer.MaterialMap->GetRawData()->Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
		}

		VulkanTexture::TransitionImage(cmd, GBuffer.AlbedoMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.NormalMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.MaterialMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		//VulkanTexture::TransitionImage(cmd, GBuffer.PositionMap->GetRawData()->Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.DepthMap->GetRawData()->Image, VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		//SE_CORE_WARN("GBUFFER PASS");
		GBufferPass(cmd);

		
		// prepare GBuffer for lighting
		//SE_CORE_WARN("LIGHTING PASS");
		VulkanTexture::TransitionImage(cmd, GBuffer.AlbedoMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.NormalMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.MaterialMap->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VulkanTexture::TransitionImage(cmd, GBuffer.DepthMap->GetRawData()->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//SE_CORE_WARN("LIGHTING PASS");
		LightingPass(cmd);
	}

	void VulkanRenderer::GenerateBRDFLut() {

		// prepare command buffer
		{
			VK_CHECK(vkResetFences(Device.Device, 1, &BRDFLutPipeline.GenFence));
			VK_CHECK(vkResetCommandBuffer(BRDFLutPipeline.Commandbuffer, 0));

			VkCommandBufferBeginInfo cmdBeginInfo = VulkanTools::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VK_CHECK(vkBeginCommandBuffer(BRDFLutPipeline.Commandbuffer, &cmdBeginInfo));
		}

		VulkanTexture::TransitionImage(BRDFLutPipeline.Commandbuffer, BRDFLutPipeline.BRDFLutTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BRDFLutPass(BRDFLutPipeline.Commandbuffer);

		VulkanTexture::TransitionImage(BRDFLutPipeline.Commandbuffer, BRDFLutPipeline.BRDFLutTexture->GetRawData()->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// end and submit command buffer
		{
			VK_CHECK(vkEndCommandBuffer(BRDFLutPipeline.Commandbuffer));

			VkCommandBufferSubmitInfo cmdInfo = VulkanTools::CommandBufferSubmitInfo(BRDFLutPipeline.Commandbuffer);

			VkSubmitInfo2 submit = VulkanTools::SubmitInfo(&cmdInfo, nullptr, nullptr);

			VK_CHECK(vkQueueSubmit2(Device.Queues.GraphicsQueue, 1, &submit, BRDFLutPipeline.GenFence));
		}

		// wait till texture is generated
		VK_CHECK(vkWaitForFences(Device.Device, 1, &BRDFLutPipeline.GenFence, true, 1000000000));
	}

	void VulkanRenderer::BRDFLutPass(VkCommandBuffer cmd) {

		VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(BRDFLutPipeline.BRDFLutTexture->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkExtent2D rExtent{};
		rExtent.width = BRDFLutPipelineData::TextureSize;
		rExtent.height = BRDFLutPipelineData::TextureSize;

		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(rExtent, &colorAttachment, 1, VK_NULL_HANDLE);
		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, BRDFLutPipeline.Pipeline);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)rExtent.width;
		viewport.height = (float)rExtent.height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = rExtent.width;
		scissor.extent.height = rExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// draw cube with skybox
		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRendering(cmd);
	}

	void VulkanRenderer::GBufferPass(VkCommandBuffer cmd) {

		// reset counters
		Stats::Data.DrawcallCount = 0;
		Stats::Data.TriangleCount = 0;

		std::vector<uint32_t> opaque_Draws;
		opaque_Draws.reserve(MainDrawContext.OpaqueSurfaces.size());

		for (uint32_t i = 0; i < MainDrawContext.OpaqueSurfaces.size(); i++) {

			if (IsVisible(MainDrawContext.OpaqueSurfaces[i], SceneData.ViewProj)) {
				opaque_Draws.push_back(i);
			}
		}

		// sort the opaque surfaces by material and mesh
		std::sort(opaque_Draws.begin(), opaque_Draws.end(), [&](const auto& iA, const auto& iB) {

			const RenderObject& A = MainDrawContext.OpaqueSurfaces[iA];
			const RenderObject& B = MainDrawContext.OpaqueSurfaces[iB];

			if (A.Material == B.Material) {
				return A.IndexBuffer < B.IndexBuffer;
			}
			else {
				return A.Material < B.Material;
			}
			});

		// render
		VkRenderingAttachmentInfo depthAttachment = VulkanTools::DepthAttachmentInfo(GBuffer.DepthMap->GetRawData()->View, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		std::array<VkRenderingAttachmentInfo, 3> colorAttachments;
		colorAttachments[0] = VulkanTools::AttachmentInfo(GBuffer.AlbedoMap->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		colorAttachments[1] = VulkanTools::AttachmentInfo(GBuffer.NormalMap->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		//colorAttachments[2] = VulkanTools::AttachmentInfo(GBuffer.PositionMap->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		colorAttachments[2] = VulkanTools::AttachmentInfo(GBuffer.MaterialMap->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(DrawExtent, colorAttachments.data(), 3, &depthAttachment);
		vkCmdBeginRendering(cmd, &renderInfo);

		Ref<Material> lastMaterial = nullptr;
		VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

		auto draw = [&](const RenderObject& draw) {

			VulkanMaterialData* matData = (VulkanMaterialData*)draw.Material->GetData();

			if (draw.Material != lastMaterial) {

				lastMaterial = draw.Material;

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->Pipeline.Pipeline);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->Pipeline.Layout, 0, 1, &MainDrawContext.FrameSceneDataSet, 0, nullptr);

				VkViewport viewport = {};
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (float)DrawExtent.width;
				viewport.height = (float)DrawExtent.height;
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;

				vkCmdSetViewport(cmd, 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent.width = DrawExtent.width;
				scissor.extent.height = DrawExtent.height;

				vkCmdSetScissor(cmd, 0, 1, &scissor);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->Pipeline.Layout, 1, 1, &GlobalMaterialManager.DataSet, 0, nullptr);
			}

			if (draw.IndexBuffer != lastIndexBuffer) {

				lastIndexBuffer = draw.IndexBuffer;
				vkCmdBindIndexBuffer(cmd, draw.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
			}

			VulkanMaterialManager::MaterialPushConstants pushConstants;
			pushConstants.BufIndex = matData->BufIndex;
			pushConstants.VertexBuffer = draw.VertexBufferAddress;
			pushConstants.WorldMatrix = draw.Transform;

			vkCmdPushConstants(cmd, matData->Pipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VulkanMaterialManager::MaterialPushConstants), &pushConstants);

			vkCmdDrawIndexed(cmd, draw.IndexCount, 1, draw.FirstIndex, 0, 0);

			// stats
			//E_CORE_INFO("DRAW");
			Stats::Data.DrawcallCount++;
			Stats::Data.TriangleCount += draw.IndexCount / 3;
		};

		//SE_CORE_INFO("{}", opaque_Draws.size());
		for (auto& r : opaque_Draws) {
			draw(MainDrawContext.OpaqueSurfaces[r]);
		}

		//for (auto& r : mainDrawContext.TransparentSurfaces) {
		//	draw(r);
		//}

		// cleanup resources, since they are no longer used
		MainDrawContext.OpaqueSurfaces.clear();
		MainDrawContext.TransparentSurfaces.clear();

		vkCmdEndRendering(cmd);
	}

	void VulkanRenderer::SkyboxPass(VkCommandBuffer cmd) {

		VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(DrawTexture->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(DrawExtent, &colorAttachment, 1, VK_NULL_HANDLE);
		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyboxPipeline.Pipeline);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyboxPipeline.PipelineLayout, 0, 1, &MainDrawContext.FrameSceneDataSet, 0, nullptr);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)DrawExtent.width;
		viewport.height = (float)DrawExtent.height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = DrawExtent.width;
		scissor.extent.height = DrawExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyboxPipeline.PipelineLayout, 1, 1, &SkyboxPipeline.Set, 0, nullptr);

		// draw cube with skybox
		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRendering(cmd);
	}

	void VulkanRenderer::LightingPass(VkCommandBuffer cmd) {

		VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(DrawTexture->GetRawData()->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(DrawExtent, &colorAttachment, 1, VK_NULL_HANDLE);
		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, LightingPipeline.Pipeline);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, LightingPipeline.PipelineLayout, 0, 1, &MainDrawContext.FrameSceneDataSet, 0, nullptr);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)DrawExtent.width;
		viewport.height = (float)DrawExtent.height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = DrawExtent.width;
		scissor.extent.height = DrawExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, LightingPipeline.PipelineLayout, 1, 1, &LightingPipeline.Set, 0, nullptr);

		// draw fullscreen quad
		vkCmdDraw(cmd, 6, 1, 0, 0);

		vkCmdEndRendering(cmd);

		// reset counter
		MainDrawContext.LightCount = 0;
	}

	void VulkanRenderer::Draw() {

		// update only, if there is camera
		if (MainCamera)
			UpdateScene();

		EXECUTION_TIME_PROFILER_START

		VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;
		uint32_t swapchainImageIndex;

		if (!BeginFrame(cmd, &swapchainImageIndex))
			return;

		// Main Draw Logic
		if (RendererMode == RendererModeDefault)
			DrawDefault(cmd, swapchainImageIndex);

		else if (RendererMode == RendererModeViewport)
			DrawViewport(cmd, swapchainImageIndex);

		// Draw ImGui
		{
			VulkanTexture::TransitionImage(cmd, Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			DrawImGui(cmd, Swapchain.ImageViews[swapchainImageIndex]);

			VulkanTexture::TransitionImage(cmd, Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}

		EndFrame(cmd, swapchainImageIndex);

		EXECUTION_TIME_PROFILER_END

		Stats::Data.RenderTime = GET_EXECUTION_TIME_MS;
	}
}