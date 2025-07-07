#include <Platforms/Vulkan/VulkanGfxDevice.h>
#include <Platforms/Vulkan/VulkanRenderResource.h>
#include <Platforms/Vulkan/VulkanTools.h>
#include <Platforms/Vulkan/VulkanPipeline.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <Engine/Core/Timestep.h>


namespace Spike {

	constexpr bool bUseValidationLayers = true;

	VulkanGfxDevice::VulkanGfxDevice(Window* window, bool useImgui) {

		m_Device.Init(window, bUseValidationLayers);
		m_Swapchain.Init(&m_Device, window->GetWidth(), window->GetHeight());

		InitCommands();
		InitSyncStructures();
		InitDescriptors();
		InitImGui((SDL_Window*)window->GetNativeWindow());
		InitDefaultData();
		InitPipelines();
		CreateBRDFLutTexture();

		m_MaterialManager.Init(&m_Device, 1000);
		//m_GuiTextureManager.Init(50);
	}

	VulkanGfxDevice::~VulkanGfxDevice() {

		vkDeviceWaitIdle(m_Device.Device);

		DestroyTexture2DGPUData(m_BRDFLutTexture);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			vkDestroyCommandPool(m_Device.Device, m_Frames[i].CommandPool, nullptr);

			// destroy sync objects
			vkDestroyFence(m_Device.Device, m_Frames[i].RenderFence, nullptr);
			vkDestroySemaphore(m_Device.Device, m_Frames[i].RenderSemaphore, nullptr);
			vkDestroySemaphore(m_Device.Device, m_Frames[i].SwapchainSemaphore, nullptr);

			m_Frames[i].ExecutionQueue.Flush();
		}

		m_MainDeletionQueue.ReversedFlush();

		m_MaterialManager.Cleanup(&m_Device);

		m_Swapchain.Destroy(&m_Device);
		m_Device.Destroy();
	}

	void VulkanGfxDevice::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func) {

		VK_CHECK(vkResetFences(m_Device.Device, 1, &m_ImmFence));
		VK_CHECK(vkResetCommandBuffer(m_ImmCommandBuffer, 0));

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanTools::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VK_CHECK(vkBeginCommandBuffer(m_ImmCommandBuffer, &cmdBeginInfo));

		func(m_ImmCommandBuffer);

		VK_CHECK(vkEndCommandBuffer(m_ImmCommandBuffer));

		VkCommandBufferSubmitInfo cmdInfo = VulkanTools::CommandBufferSubmitInfo(m_ImmCommandBuffer);
		VkSubmitInfo2 submit = VulkanTools::SubmitInfo(&cmdInfo, nullptr, nullptr);

		VK_CHECK(vkQueueSubmit2(m_Device.Queues.GraphicsQueue, 1, &submit, m_ImmFence));

		VK_CHECK(vkWaitForFences(m_Device.Device, 1, &m_ImmFence, true, 9999999999));
	}

	void VulkanGfxDevice::InitPipelines() {

		InitSkyboxPipeline();
		InitLightPipeline();
		InitDepthPyramidPipeline();
		InitCullPipeline();
		InitCubeTextureGenPipeline();
		InitIrradianceGenPipeline();
		InitRadianceGenPipeline();
		InitBRDFLutPipeline();
	}

	void VulkanGfxDevice::InitCommands() {

		// create a command pool for commands submitted to the graphics queue
		VkCommandPoolCreateInfo commandPoolInfo = VulkanTools::CommandPoolCreateInfo(m_Device.Queues.GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			VK_CHECK(vkCreateCommandPool(m_Device.Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool));

			// allocate the default command buffer thta we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = VulkanTools::CommandBufferAllocInfo(m_Frames[i].CommandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(m_Device.Device, &cmdAllocInfo, &m_Frames[i].MainCommandBuffer));
		}

		// immediate submit 
		VK_CHECK(vkCreateCommandPool(m_Device.Device, &commandPoolInfo, nullptr, &m_ImmCommandPool));

		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanTools::CommandBufferAllocInfo(m_ImmCommandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_Device.Device, &cmdAllocInfo, &m_ImmCommandBuffer));

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyCommandPool(m_Device.Device, m_ImmCommandPool, nullptr);
			});
	}

	void VulkanGfxDevice::InitSyncStructures() {

		VkFenceCreateInfo fenceCreateInfo = VulkanTools::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanTools::SemaphoreCreateInfo();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(m_Device.Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence));

			VK_CHECK(vkCreateSemaphore(m_Device.Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(m_Device.Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].RenderSemaphore));
		}

		// immediate fence
		VK_CHECK(vkCreateFence(m_Device.Device, &fenceCreateInfo, nullptr, &m_ImmFence));

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyFence(m_Device.Device, m_ImmFence, nullptr);
			});
	}

	void VulkanGfxDevice::InitDescriptors() {

		std::array<DescriptorAllocator::PoolSizeRatio, 4> sizes = {

			DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .Ratio = 15},
			DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .Ratio = 15},
			DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .Ratio = 15},
			DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .Ratio = 15}
		};

		m_DescriptorAllocator.InitPool(m_Device.Device, 15, sizes);

		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);                       // scene data buffer
			builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // draw commands buffer
			builder.AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // draw count buffer
			builder.AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // scene object meta buffer
			builder.AddBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // batch offsets buffer
			builder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // depth pyramid image
			builder.AddBinding(6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);                       // cull data buffer 
			builder.AddBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // last frame visibility buffer
			builder.AddBinding(8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // current frame visibility buffer

			m_GeometrySetLayout = builder.Build(m_Device.Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
		}

		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);                       // scene data buffer
			builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // albedo image
			builder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // normal image
			builder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // material image
			builder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // depth image
			builder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // environment image
			builder.AddBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);                       // lights buffer
			builder.AddBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // irradiance image
			builder.AddBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);               // brdf lut image

			m_LightingSetLayout = builder.Build(m_Device.Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			// create a descriptor pool
			std::array<DescriptorAllocator::PoolSizeRatio, 4> frameSizes = {

			    DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .Ratio = 20 * MAX_SCENE_DRAWS_PER_FRAME},
			    DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .Ratio = 20 * MAX_SCENE_DRAWS_PER_FRAME},
				DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .Ratio = 20 * MAX_SCENE_DRAWS_PER_FRAME},
			    DescriptorAllocator::PoolSizeRatio{.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .Ratio = 20 * MAX_SCENE_DRAWS_PER_FRAME}
			};

			m_Frames[i].FrameAllocator = DescriptorAllocator{};
			m_Frames[i].FrameAllocator.InitPool(m_Device.Device, 20 * MAX_SCENE_DRAWS_PER_FRAME, frameSizes);

			m_Frames[i].SceneDataBuffer = VulkanBuffer(&m_Device, MAX_SCENE_DRAWS_PER_FRAME * sizeof(SceneDrawData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_Frames[i].LightsBuffer = VulkanBuffer(&m_Device, MAX_LIGHTS_PER_FRAME * sizeof(SceneLightProxy), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_Frames[i].DrawCommandsBuffer = VulkanBuffer(&m_Device, MAX_DRAWS_PER_FRAME * sizeof(DrawIndirectCommand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
			m_Frames[i].DrawCountBuffer = VulkanBuffer(&m_Device, MAX_MATERIALS_PER_FRAME * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
			m_Frames[i].SceneObjectMetaBuffer = VulkanBuffer(&m_Device, MAX_DRAWS_PER_FRAME * sizeof(SceneObjectMetadata), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_Frames[i].BatchOffsetsBuffer = VulkanBuffer(&m_Device, MAX_MATERIALS_PER_FRAME * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_Frames[i].CullDataBuffer = VulkanBuffer(&m_Device, MAX_SCENE_DRAWS_PER_FRAME * sizeof(CullData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_Frames[i].VisibilityBuffer = VulkanBuffer(&m_Device, MAX_DRAWS_PER_FRAME * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

			ImmediateSubmit([this, i](VkCommandBuffer cmd) {

				VulkanTools::FillBuffer(cmd, m_Frames[i].VisibilityBuffer.Buffer, VK_WHOLE_SIZE, 0, 0, VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
				});

			m_MainDeletionQueue.PushFunction([this, i]() {

				m_Frames[i].FrameAllocator.DestroyPool(m_Device.Device);
				m_Frames[i].SceneDataBuffer.Destroy(&m_Device);
				m_Frames[i].LightsBuffer.Destroy(&m_Device);
				m_Frames[i].DrawCommandsBuffer.Destroy(&m_Device);
				m_Frames[i].DrawCountBuffer.Destroy(&m_Device);
				m_Frames[i].SceneObjectMetaBuffer.Destroy(&m_Device);
				m_Frames[i].BatchOffsetsBuffer.Destroy(&m_Device);
				m_Frames[i].CullDataBuffer.Destroy(&m_Device);
				m_Frames[i].VisibilityBuffer.Destroy(&m_Device);
				});
		}

		m_MainDeletionQueue.PushFunction([this]() {

			m_DescriptorAllocator.DestroyPool(m_Device.Device);
			vkDestroyDescriptorSetLayout(m_Device.Device, m_GeometrySetLayout, nullptr);
			vkDestroyDescriptorSetLayout(m_Device.Device, m_LightingSetLayout, nullptr);
			});
	}

	void VulkanGfxDevice::InitImGui(SDL_Window* window) {

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
		VK_CHECK(vkCreateDescriptorPool(m_Device.Device, &pool_Info, nullptr, &imguiPool));

		ImGui::CreateContext();

		ImGui_ImplSDL2_InitForVulkan(window);

		ImGui_ImplVulkan_InitInfo init_Info = {};
		init_Info.Instance = m_Device.Instance;
		init_Info.PhysicalDevice = m_Device.PhysicalDevice;
		init_Info.Device = m_Device.Device;
		init_Info.Queue = m_Device.Queues.GraphicsQueue;
		init_Info.DescriptorPool = imguiPool;
		init_Info.MinImageCount = 3;
		init_Info.ImageCount = 3;
		init_Info.UseDynamicRendering = true;

		init_Info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		init_Info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_Info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_Swapchain.Format;

		init_Info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_Info);

		ImGui_ImplVulkan_CreateFontsTexture();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		m_GuiTextureManager.Init(50);


		m_MainDeletionQueue.PushFunction([this, imguiPool]() {

			m_GuiTextureManager.Cleanup();
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(m_Device.Device, imguiPool, nullptr);
			});
	}

	void VulkanGfxDevice::InitDefaultData() {

		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		sampl.magFilter = VK_FILTER_NEAREST;
		sampl.minFilter = VK_FILTER_NEAREST;

		vkCreateSampler(m_Device.Device, &sampl, nullptr, &m_DefSamplerNearest);

		sampl.magFilter = VK_FILTER_LINEAR;
		sampl.minFilter = VK_FILTER_LINEAR;

		sampl.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampl.maxLod = 50;

		vkCreateSampler(m_Device.Device, &sampl, nullptr, &m_DefSamplerLinear);

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroySampler(m_Device.Device, m_DefSamplerNearest, nullptr);
			vkDestroySampler(m_Device.Device, m_DefSamplerLinear, nullptr);
			});
	}

	void VulkanGfxDevice::InitSkyboxPipeline() {

		// init pipeline
		{
			VkPushConstantRange matrixRange{};
			matrixRange.offset = 0;
			matrixRange.size = sizeof(SkyboxData);
			matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayout layouts[] = { m_LightingSetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = &matrixRange;
			layout_Info.pushConstantRangeCount = 1;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_SkyboxPipeline.PipelineLayout));

			VulkanShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/Skybox.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/Skybox.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			VkFormat colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

			pipelineBulder.SetColorAttachments(&colorAttachmentFormat, 1);
			pipelineBulder.SetDepthFormat(VK_FORMAT_D32_SFLOAT);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = m_SkyboxPipeline.PipelineLayout;
			pipelineBulder.DisableBlending();
			pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_SkyboxPipeline.Pipeline = pipelineBulder.BuildPipeline(m_Device.Device);

			shader.Destroy(&m_Device);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_SkyboxPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_SkyboxPipeline.Pipeline, nullptr);
			});
	}

	void VulkanGfxDevice::InitLightPipeline() {

		// init pipeline
		{
			VkPushConstantRange matrixRange{};
			matrixRange.offset = 0;
			matrixRange.size = sizeof(LightingData); // scene data offset, lights offset, lights count
			matrixRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayout layouts[] = { m_LightingSetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = &matrixRange;
			layout_Info.pushConstantRangeCount = 1;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_LightingPipeline.PipelineLayout));

			VulkanShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/LightingPBR.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/LightingPBR.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			VkFormat colorAttachmentFormats[1] = { VK_FORMAT_R16G16B16A16_SFLOAT };

			pipelineBulder.SetColorAttachments(colorAttachmentFormats, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = m_LightingPipeline.PipelineLayout;
			pipelineBulder.DisableBlending();
			//pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_LightingPipeline.Pipeline = pipelineBulder.BuildPipeline(m_Device.Device);

			shader.Destroy(&m_Device);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_LightingPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_LightingPipeline.Pipeline, nullptr);
			});
	}

	void VulkanGfxDevice::InitDepthPyramidPipeline() {

		// build set layout
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);                  // dest image
			builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);         // source img
			m_DepthPyramidPipeline.SetLayout = builder.Build(m_Device.Device, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		// init pipeline
		{
			VkPushConstantRange matrixRange{};
			matrixRange.offset = 0;
			matrixRange.size = sizeof(DepthReduceData); 
			matrixRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			VkDescriptorSetLayout layouts[] = { m_DepthPyramidPipeline.SetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = &matrixRange;
			layout_Info.pushConstantRangeCount = 1;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_DepthPyramidPipeline.PipelineLayout));

			VulkanComputeShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/DepthPyramid.comp.spv");

			VkPipelineShaderStageCreateInfo stageInfo = VulkanTools::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, shader.ComputeModule);

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.stage = stageInfo;
			pipelineInfo.layout = m_DepthPyramidPipeline.PipelineLayout;

			VK_CHECK(vkCreateComputePipelines(m_Device.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_DepthPyramidPipeline.Pipeline));

			shader.Destroy(&m_Device);
		}

		// create sampler
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.minLod = 0;
			createInfo.maxLod = 16.f;

			VkSamplerReductionModeCreateInfoEXT createInfoReduction = {};
			createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;
			createInfoReduction.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
			createInfo.pNext = &createInfoReduction;

			VK_CHECK(vkCreateSampler(m_Device.Device, &createInfo, 0, &m_DepthPyramidPipeline.DepthPyramidSampler));
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_DepthPyramidPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_DepthPyramidPipeline.Pipeline, nullptr);
			vkDestroyDescriptorSetLayout(m_Device.Device, m_DepthPyramidPipeline.SetLayout, nullptr);
			vkDestroySampler(m_Device.Device, m_DepthPyramidPipeline.DepthPyramidSampler, nullptr);
			});
	}

	void VulkanGfxDevice::InitCullPipeline() {

		// init pipeline
		{
			VkPushConstantRange matrixRange{};
			matrixRange.offset = 0;
			matrixRange.size = sizeof(CullPushData);
			matrixRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			VkDescriptorSetLayout layouts[] = { m_GeometrySetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = &matrixRange;
			layout_Info.pushConstantRangeCount = 1;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_CullPipeline.PipelineLayout));

			VulkanComputeShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/IndirectCull.comp.spv");

			VkPipelineShaderStageCreateInfo stageInfo = VulkanTools::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, shader.ComputeModule);

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.stage = stageInfo;
			pipelineInfo.layout = m_CullPipeline.PipelineLayout;

			VK_CHECK(vkCreateComputePipelines(m_Device.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_CullPipeline.Pipeline));

			shader.Destroy(&m_Device);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_CullPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_CullPipeline.Pipeline, nullptr);
			});
	}

	void VulkanGfxDevice::InitCubeTextureGenPipeline() {

		// create desc set layout
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);           // dest cube texture
			builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // source rectangle texture
			m_CubeTextureGenPipeline.SetLayout = builder.Build(m_Device.Device, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		// init pipeline
		{
			VkDescriptorSetLayout layouts[] = { m_CubeTextureGenPipeline.SetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = nullptr;
			layout_Info.pushConstantRangeCount = 0;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_CubeTextureGenPipeline.PipelineLayout));

			VulkanComputeShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/CubeMapGen.comp.spv");

			VkPipelineShaderStageCreateInfo stageInfo = VulkanTools::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, shader.ComputeModule);

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.stage = stageInfo;
			pipelineInfo.layout = m_CubeTextureGenPipeline.PipelineLayout;

			VK_CHECK(vkCreateComputePipelines(m_Device.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_CubeTextureGenPipeline.Pipeline));

			shader.Destroy(&m_Device);
		}

		// create desc set 
		{
			m_CubeTextureGenPipeline.Set = m_DescriptorAllocator.Allocate(m_Device.Device, m_CubeTextureGenPipeline.SetLayout);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_CubeTextureGenPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_CubeTextureGenPipeline.Pipeline, nullptr);
			vkDestroyDescriptorSetLayout(m_Device.Device, m_CubeTextureGenPipeline.SetLayout, nullptr);
			});
	}

	void VulkanGfxDevice::InitIrradianceGenPipeline() {

		// create desc set layout
		{
			DescriptorLayoutBuilder builder;
			//builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // sampler image
			m_IrradianceGenPipeline.SetLayout = builder.Build(m_Device.Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		// init pipeline
		{
			VkPushConstantRange matrixRange{};
			matrixRange.offset = 0;
			matrixRange.size = sizeof(CubeFilteringData);
			matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayout layouts[] = { m_IrradianceGenPipeline.SetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = &matrixRange;
			layout_Info.pushConstantRangeCount = 1;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_IrradianceGenPipeline.PipelineLayout));

			VulkanShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/CubeMap.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/IrradianceMap.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			VkFormat colorAttachmentFormats[1] = { VK_FORMAT_R32G32B32A32_SFLOAT };

			pipelineBulder.SetColorAttachments(colorAttachmentFormats, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = m_IrradianceGenPipeline.PipelineLayout;
			pipelineBulder.DisableBlending();
			//pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_IrradianceGenPipeline.Pipeline = pipelineBulder.BuildPipeline(m_Device.Device);

			shader.Destroy(&m_Device);
		}

		// create desc set 
		{
			m_IrradianceGenPipeline.Set = m_DescriptorAllocator.Allocate(m_Device.Device, m_IrradianceGenPipeline.SetLayout);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_IrradianceGenPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_IrradianceGenPipeline.Pipeline, nullptr);
			vkDestroyDescriptorSetLayout(m_Device.Device, m_IrradianceGenPipeline.SetLayout, nullptr);
			});
	}

	void VulkanGfxDevice::InitRadianceGenPipeline() {

		// create desc set layout
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // sampler image
			m_RadianceGenPipeline.SetLayout = builder.Build(m_Device.Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		// init pipeline
		{
			VkPushConstantRange matrixRange{};
			matrixRange.offset = 0;
			matrixRange.size = sizeof(CubeFilteringData); 
			matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayout layouts[] = { m_RadianceGenPipeline.SetLayout };

			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 1;
			layout_Info.pSetLayouts = layouts;
			layout_Info.pPushConstantRanges = &matrixRange;
			layout_Info.pushConstantRangeCount = 1;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_RadianceGenPipeline.PipelineLayout));

			VulkanShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/CubeMap.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/EnvironmentMap.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			VkFormat colorAttachmentFormats[1] = { VK_FORMAT_R32G32B32A32_SFLOAT };

			pipelineBulder.SetColorAttachments(colorAttachmentFormats, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = m_RadianceGenPipeline.PipelineLayout;
			pipelineBulder.DisableBlending();
			//pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_RadianceGenPipeline.Pipeline = pipelineBulder.BuildPipeline(m_Device.Device);

			shader.Destroy(&m_Device);
		}

		// create desc set
		{
			m_RadianceGenPipeline.Set = m_DescriptorAllocator.Allocate(m_Device.Device, m_RadianceGenPipeline.SetLayout);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_RadianceGenPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_RadianceGenPipeline.Pipeline, nullptr);
			vkDestroyDescriptorSetLayout(m_Device.Device, m_RadianceGenPipeline.SetLayout, nullptr);
			});
	}

	void VulkanGfxDevice::InitBRDFLutPipeline() {

		// init pipeline
		{
			VkPipelineLayoutCreateInfo layout_Info = VulkanTools::PipelineLayoutCreateInfo();
			layout_Info.setLayoutCount = 0;
			layout_Info.pSetLayouts = nullptr;
			layout_Info.pPushConstantRanges = nullptr;
			layout_Info.pushConstantRangeCount = 0;

			VK_CHECK(vkCreatePipelineLayout(m_Device.Device, &layout_Info, nullptr, &m_BRDFLutPipeline.PipelineLayout));

			VulkanShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/BRDFLut.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/BRDFLut.frag.spv");

			PipelineBuilder pipelineBulder;
			pipelineBulder.SetShaders(shader.VertexModule, shader.FragmentModule);
			pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
			pipelineBulder.SetMultisamplingNone();

			VkFormat colorAttachmentFormats[1] = { VK_FORMAT_R16G16_SFLOAT };

			pipelineBulder.SetColorAttachments(colorAttachmentFormats, 1);
			//pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

			pipelineBulder.PipelineLayout = m_BRDFLutPipeline.PipelineLayout;
			pipelineBulder.DisableBlending();
			//pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_BRDFLutPipeline.Pipeline = pipelineBulder.BuildPipeline(m_Device.Device);

			shader.Destroy(&m_Device);
		}

		m_MainDeletionQueue.PushFunction([this]() {

			vkDestroyPipelineLayout(m_Device.Device, m_BRDFLutPipeline.PipelineLayout, nullptr);
			vkDestroyPipeline(m_Device.Device, m_BRDFLutPipeline.Pipeline, nullptr);
			});
	}

	void VulkanGfxDevice::CreateBRDFLutTexture() {

		static constexpr uint32_t BRDF_LUT_SIZE = 256;

		m_BRDFLutTexture = new VulkanTexture2DGPUData();
		CreateVulkanTexture2D(BRDF_LUT_SIZE, BRDF_LUT_SIZE, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1, m_BRDFLutTexture);

		ImmediateSubmit([&](VkCommandBuffer cmd) {

			VulkanTools::TransitionImage(cmd, m_BRDFLutTexture->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(m_BRDFLutTexture->NativeGPUData.View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkRenderingInfo renderInfo = VulkanTools::RenderingInfo({BRDF_LUT_SIZE, BRDF_LUT_SIZE}, &colorAttachment, 1, VK_NULL_HANDLE);
			vkCmdBeginRendering(cmd, &renderInfo);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BRDFLutPipeline.Pipeline);

			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)BRDF_LUT_SIZE;
			viewport.height = (float)BRDF_LUT_SIZE;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = BRDF_LUT_SIZE;
			scissor.extent.height = BRDF_LUT_SIZE;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			vkCmdDraw(cmd, 3, 1, 0, 0);

			vkCmdEndRendering(cmd);

			VulkanTools::TransitionImage(cmd, m_BRDFLutTexture->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});
	}

	void VulkanGfxDevice::CreateVulkanTexture2D(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usageFlags, uint32_t numMips, VulkanTexture2DGPUData* outTexture) {

		VkImageCreateInfo img_info = VulkanTools::ImageCreateInfo(format, usageFlags, {width, height, 1});
		img_info.mipLevels = numMips;

		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(m_Device.Allocator, &img_info, &allocinfo, &outTexture->NativeGPUData.Image, &outTexture->NativeGPUData.Allocation, nullptr));

		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;

		// build a image-view for the image
		VkImageViewCreateInfo view_info = VulkanTools::ImageviewCreateInfo(format, outTexture->NativeGPUData.Image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		VK_CHECK(vkCreateImageView(m_Device.Device, &view_info, nullptr, &outTexture->NativeGPUData.View));
	}

	ResourceGPUData* VulkanGfxDevice::CreateTexture2DGPUData(uint32_t width, uint32_t height, ETextureFormat format, ETextureUsageFlags usageFlags, bool mipmap, void* pixelData) {

		VulkanTexture2DGPUData* texData = new VulkanTexture2DGPUData();

		VkFormat vFormat = VulkanTools::TextureFormatToVulkan(format);
		VkImageUsageFlags vUsageFlags = VulkanTools::TextureUsageFlagsToVulkan(usageFlags);

		uint32_t numMips = 1;
		if (mipmap)
			numMips = uint32_t(std::floor(std::log2(std::max(width, height)))) + 1;

		CreateVulkanTexture2D(width, height, vFormat, vUsageFlags, numMips, texData);

		// populate texture with pixel data
		if (pixelData) {

			size_t data_size = 0;
			if (vFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
				data_size = width * height * 16;
			}
			else {
				data_size = width * height * 4;
			}

			VulkanBuffer uploadbuffer(&m_Device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			memcpy(uploadbuffer.AllocationInfo.pMappedData, pixelData, data_size);

			ImmediateSubmit([&](VkCommandBuffer cmd) {

				VulkanTools::TransitionImage(cmd, texData->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = {width, height, 1};

				// copy the buffer into the image
				vkCmdCopyBufferToImage(cmd, uploadbuffer.Buffer, texData->NativeGPUData.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
					&copyRegion);

				if (mipmap) {
					VulkanTools::GenerateMipmaps(cmd, texData->NativeGPUData.Image, VkExtent2D { width, height });
				}
				else {
					VulkanTools::TransitionImage(cmd, texData->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}
				});

			uploadbuffer.Destroy(&m_Device);
		}

		return texData;
	}

	void VulkanGfxDevice::DestroyTexture2DGPUData(ResourceGPUData* data) {

		GetCurrentFrame().ExecutionQueue.PushFunction([this, data]() {

			VulkanTextureNativeData* nativeData = (VulkanTextureNativeData*)data->GetNativeData();

			if (nativeData->View != VK_NULL_HANDLE) {

				vkDestroyImageView(m_Device.Device, nativeData->View, nullptr);
			}

			if (nativeData->Image != VK_NULL_HANDLE) {

				vmaDestroyImage(m_Device.Allocator, nativeData->Image, nativeData->Allocation);
			}

			delete data;
			});
	}

	void VulkanGfxDevice::CreateVulkanCubeTexture(uint32_t size, VkFormat format, VkImageUsageFlags usageFlags, uint32_t numMips, VulkanCubeTextureGPUData* outTexture) {

		VkImageCreateInfo img_info = VulkanTools::CubeImageCreateInfo(format, usageFlags, size);
		img_info.mipLevels = numMips;

		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(m_Device.Allocator, &img_info, &allocinfo, &outTexture->NativeGPUData.Image, &outTexture->NativeGPUData.Allocation, nullptr));

		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;

		// build a image-view for the image
		VkImageViewCreateInfo view_info = VulkanTools::CubeImageviewCreateInfo(format, outTexture->NativeGPUData.Image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		VK_CHECK(vkCreateImageView(m_Device.Device, &view_info, nullptr, &outTexture->NativeGPUData.View));
	}

	ResourceGPUData* VulkanGfxDevice::CreateCubeTextureGPUData(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture) {

		VulkanCubeTextureGPUData* texData = new VulkanCubeTextureGPUData();

		VkFormat vFormat = VulkanTools::TextureFormatToVulkan(format);
		VkImageUsageFlags vUsageFlags = VulkanTools::TextureUsageFlagsToVulkan(usageFlags);

		CreateVulkanCubeTexture(size, vFormat, vUsageFlags, 1, texData);
		VulkanTextureNativeData* samplerNativeData = (VulkanTextureNativeData*)samplerTexture->GetGPUData()->GetNativeData();

		ImmediateSubmit([&](VkCommandBuffer cmd) {

			VulkanTools::TransitionImageCube(cmd, texData->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			VulkanTools::TransitionImage(cmd, samplerNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			// write desc set
			{
				DescriptorWriter writer;
				writer.WriteImage(0, texData->NativeGPUData.View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				writer.WriteImage(1, samplerNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.UpdateSet(m_Device.Device, m_CubeTextureGenPipeline.Set);
			}

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CubeTextureGenPipeline.Pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CubeTextureGenPipeline.PipelineLayout, 0, 1, &m_CubeTextureGenPipeline.Set, 0, nullptr);

			uint32_t groupCount = VulkanTools::GetComputeGroupCount(size, 8);
			vkCmdDispatch(cmd, groupCount, groupCount, 6);

			VulkanTools::TransitionImageCube(cmd, texData->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		return texData;
	}

	ResourceGPUData* VulkanGfxDevice::CreateFilteredCubeTextureGPUData(uint32_t size, ETextureFormat format, ETextureUsageFlags usageFlags, TextureResource* samplerTexture, ECubeTextureFilterMode filterMode) {

		//if (!(format & EUsageFlagTransferDst)) assert(false);

		VulkanCubeTextureGPUData* texData = new VulkanCubeTextureGPUData();

		VkFormat vFormat = VulkanTools::TextureFormatToVulkan(format);
		VkImageUsageFlags vUsageFlags = VulkanTools::TextureUsageFlagsToVulkan(usageFlags);

		//if (!(vFormat & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) assert(false);

		uint32_t numMips = 1;
		if (filterMode == EFilterRadiance)
		    numMips = uint32_t(std::floor(std::log2(size))) + 1;

		CreateVulkanCubeTexture(size, vFormat, vUsageFlags, numMips, texData);
		VulkanCubeTextureNativeData* samplerNativeData = (VulkanCubeTextureNativeData*)samplerTexture->GetGPUData()->GetNativeData();

		//ResourceGPUData* offscreen = CreateTexture2DGPUData(size, size, format, EUsageFlagColorAttachment | EUsageFlagTransferSrc, false, nullptr);
		//VulkanTextureNativeData* offscreenData = (VulkanTextureNativeData*)offscreen->GetNativeData();

		VulkanTexture2DGPUData* offscreen = new VulkanTexture2DGPUData();
		CreateVulkanTexture2D(size, size, vFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1, offscreen);

		std::array<glm::mat4, 6> matrices = {

				glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};

		ImmediateSubmit([&](VkCommandBuffer cmd) {

			VulkanTools::TransitionImageCube(cmd, texData->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			//VulkanTools::TransitionImageCube(cmd, samplerNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VulkanTools::TransitionImage(cmd, offscreen->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkDescriptorSet descSet = m_IrradianceGenPipeline.Set;
			if (filterMode == EFilterRadiance)
				descSet = m_RadianceGenPipeline.Set;

			VkPipeline pipeline = m_IrradianceGenPipeline.Pipeline;
			if (filterMode == EFilterRadiance)
				pipeline = m_RadianceGenPipeline.Pipeline;

			VkPipelineLayout pipelineLayout = m_IrradianceGenPipeline.PipelineLayout;
			if (filterMode == EFilterRadiance)
				pipelineLayout = m_RadianceGenPipeline.PipelineLayout;

			// write desc set
			{
				DescriptorWriter writer;
				writer.WriteImage(0, samplerNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.UpdateSet(m_Device.Device, descSet);
			}

			for (int i = 0; i < 6; i++) {

				for (uint32_t m = 0; m < numMips; m++) {

					VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(offscreen->NativeGPUData.View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

					uint32_t mipSize = size >> m;
					if (mipSize < 1) mipSize = 1;

					VkRenderingInfo renderInfo = VulkanTools::RenderingInfo({ mipSize, mipSize }, &colorAttachment, 1, VK_NULL_HANDLE);
					vkCmdBeginRendering(cmd, &renderInfo);

					VkViewport viewport = {};
					viewport.x = 0;
					viewport.y = 0;
					viewport.width = (float)mipSize;
					viewport.height = (float)mipSize;
					viewport.minDepth = 0.f;
					viewport.maxDepth = 1.f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor = {};
					scissor.offset.x = 0;
					scissor.offset.y = 0;
					scissor.extent.width = mipSize;
					scissor.extent.height = mipSize;

					vkCmdSetScissor(cmd, 0, 1, &scissor);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);

					CubeFilteringData pushConstants{};
					pushConstants.MVP = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[i];

					if (filterMode == EFilterRadiance)
					    pushConstants.Roughness = (float)m / (float)(numMips - 1);

					vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

					vkCmdDraw(cmd, 36, 1, 0, 0);

					vkCmdEndRendering(cmd);

					// copy from offscreen image to cube texture
					{
						VulkanTools::TransitionImage(cmd, offscreen->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

						VkImageCopy2 copyRegion{};
						copyRegion.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
						copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.srcSubresource.baseArrayLayer = 0;
						copyRegion.srcSubresource.mipLevel = 0;
						copyRegion.srcSubresource.layerCount = 1;
						copyRegion.srcOffset = { 0, 0, 0 };

						copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.dstSubresource.baseArrayLayer = i;
						copyRegion.dstSubresource.mipLevel = m;
						copyRegion.dstSubresource.layerCount = 1;
						copyRegion.dstOffset = { 0, 0, 0 };

						copyRegion.extent.width = mipSize;
						copyRegion.extent.height = mipSize;
						copyRegion.extent.depth = 1;

						VkCopyImageInfo2 copyInfo{};
						copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
						copyInfo.srcImage = offscreen->NativeGPUData.Image;
						copyInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						copyInfo.dstImage = texData->NativeGPUData.Image;
						copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						copyInfo.regionCount = 1;
						copyInfo.pRegions = &copyRegion;

						vkCmdCopyImage2(cmd, &copyInfo);

						VulkanTools::TransitionImage(cmd, offscreen->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}
				}
			}

			VulkanTools::TransitionImageCube(cmd, texData->NativeGPUData.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		DestroyTexture2DGPUData(offscreen);

		return texData;
	}

	void VulkanGfxDevice::DestroyCubeTextureGPUData(ResourceGPUData* data) {

		GetCurrentFrame().ExecutionQueue.PushFunction([this, data]() {

			VulkanCubeTextureNativeData* nativeData = (VulkanCubeTextureNativeData*)data->GetNativeData();

			if (nativeData->View != VK_NULL_HANDLE) {

				vkDestroyImageView(m_Device.Device, nativeData->View, nullptr);
			}

			if (nativeData->Image != VK_NULL_HANDLE) {

				vmaDestroyImage(m_Device.Allocator, nativeData->Image, nativeData->Allocation);
			}

			delete data;
			});
	}

	ResourceGPUData* VulkanGfxDevice::CreateMaterialGPUData(EMaterialSurfaceType surfaceType) {

		VulkanMaterialGPUData* matData = new VulkanMaterialGPUData();

		uint32_t bufIndex = m_MaterialManager.GetFreeBufferIndex();

		// create new material buffer
		{
			VulkanMaterialManager::MaterialDataConstants newBuf{};

			for (int i = 0; i < 16; i++) {

				newBuf.TexIndex[i] = VulkanMaterialManager::IndexInvalid;
			}

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
			constants[bufIndex] = newBuf;

			m_MaterialManager.WriteBuffer(&m_Device, bufIndex);
		}

		matData->NativeGPUData.BufferIndex = bufIndex;

		VulkanShader shader(&m_Device, "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/GBufferPBR.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/GBufferPBR.frag.spv");

		m_MaterialManager.BuildMaterialPipeline(&matData->NativeGPUData.Pipeline, &matData->NativeGPUData.PipelineLayout, &m_Device, m_GeometrySetLayout, shader, surfaceType);

		shader.Destroy(&m_Device);

		return matData;
	}

	void VulkanGfxDevice::DestroyMaterialGPUData(ResourceGPUData* data) {

		GetCurrentFrame().ExecutionQueue.PushFunction([this, data]() {

			VulkanMaterialNativeData* nativeData = (VulkanMaterialNativeData*)data->GetNativeData();

			for (int i = 0; i < 16; i++) {

				VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
				uint32_t texIndex = constants[nativeData->BufferIndex].TexIndex[i];

				if (texIndex != VulkanMaterialManager::IndexInvalid) {

					m_MaterialManager.ReleaseTextureIndex(texIndex);
				}
			}

			m_MaterialManager.ReleaseBufferIndex(nativeData->BufferIndex);

			if (nativeData->PipelineLayout != VK_NULL_HANDLE) {

				vkDestroyPipelineLayout(m_Device.Device, nativeData->PipelineLayout, nullptr);
			}

			if (nativeData->Pipeline != VK_NULL_HANDLE) {

				vkDestroyPipeline(m_Device.Device, nativeData->Pipeline, nullptr);
			}

			delete data;
			});
	}

	ResourceGPUData* VulkanGfxDevice::CreateMeshGPUData(const std::span<Vertex>& vertices, const std::span<uint32_t>& indices) {

		VulkanMeshGPUData* meshData = new VulkanMeshGPUData();

		const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		meshData->NativeGPUData.VertexBuffer = VulkanBuffer(&m_Device, vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		meshData->NativeGPUData.IndexBuffer = VulkanBuffer(&m_Device, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		VkBufferDeviceAddressInfo vDeviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshData->NativeGPUData.VertexBuffer.Buffer };
		meshData->NativeGPUData.VertexBufferAddress = vkGetBufferDeviceAddress(m_Device.Device, &vDeviceAddressInfo);

		VkBufferDeviceAddressInfo iDeviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshData->NativeGPUData.IndexBuffer.Buffer };
		meshData->NativeGPUData.IndexBufferAddress = vkGetBufferDeviceAddress(m_Device.Device, &iDeviceAddressInfo);

		VulkanBuffer staging(&m_Device, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(m_Device.Allocator, staging.Allocation, &data);

		// copy vertex and index buffers
		memcpy(data, vertices.data(), vertexBufferSize);
		memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

		vmaUnmapMemory(m_Device.Allocator, staging.Allocation);

		ImmediateSubmit([&](VkCommandBuffer cmd) {

			VkBufferCopy vertexCopy{ 0 };
			vertexCopy.dstOffset = 0;
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vertexBufferSize;

			vkCmdCopyBuffer(cmd, staging.Buffer, meshData->NativeGPUData.VertexBuffer.Buffer, 1, &vertexCopy);

			VkBufferCopy indexCopy{ 0 };
			indexCopy.dstOffset = 0;
			indexCopy.srcOffset = vertexBufferSize;
			indexCopy.size = indexBufferSize;

			vkCmdCopyBuffer(cmd, staging.Buffer, meshData->NativeGPUData.IndexBuffer.Buffer, 1, &indexCopy);
			});

		staging.Destroy(&m_Device);

		return meshData;
	}

	void VulkanGfxDevice::DestroyMeshGPUData(ResourceGPUData* data) {

		GetCurrentFrame().ExecutionQueue.PushFunction([this, data]() {

			VulkanMeshNativeData* nativeData = (VulkanMeshNativeData*)data->GetNativeData();

			nativeData->IndexBuffer.Destroy(&m_Device);
			nativeData->VertexBuffer.Destroy(&m_Device);

			delete data;
			});
	}

	ResourceGPUData* VulkanGfxDevice::CreateDepthMapGPUData(uint32_t width, uint32_t height, uint32_t depthPyramidSize) {

		VulkanDepthMapGPUData* mapData = new VulkanDepthMapGPUData();
		uint32_t numMips  = uint32_t(std::floor(std::log2(std::max(width, height)))) + 1;

		VkExtent3D size {

			width,
			height,
			1
		};

		// generate main depth image and view
		{
			VkImageCreateInfo img_info = VulkanTools::ImageCreateInfo(VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, size);
			img_info.mipLevels = numMips;

			VmaAllocationCreateInfo allocinfo = {};
			allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// allocate and create the image
			VK_CHECK(vmaCreateImage(m_Device.Allocator, &img_info, &allocinfo, &mapData->NativeGPUData.Image, &mapData->NativeGPUData.Allocation, nullptr));

			VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;

			// build a image-view for the image
			VkImageViewCreateInfo view_info = VulkanTools::ImageviewCreateInfo(VK_FORMAT_D32_SFLOAT, mapData->NativeGPUData.Image, aspectFlag);
			view_info.subresourceRange.levelCount = numMips;

			VK_CHECK(vkCreateImageView(m_Device.Device, &view_info, nullptr, &mapData->NativeGPUData.View));
		}

		// generate depth pyramid image and views
		{
			VkExtent3D pyramidSize {

				depthPyramidSize,
				depthPyramidSize,
				1
			};

			//uint32_t numPyramidMips = uint32_t(std::floor(std::log2(depthPyramidSize))) + 1;
			uint32_t numPyramidMips = uint32_t(glm::log2(float(depthPyramidSize))) + 1;

			VkImageCreateInfo img_info = VulkanTools::ImageCreateInfo(VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, pyramidSize);
			img_info.mipLevels = numPyramidMips;

			VmaAllocationCreateInfo allocinfo = {};
			allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// allocate and create the image
			VK_CHECK(vmaCreateImage(m_Device.Allocator, &img_info, &allocinfo, &mapData->NativeGPUData.DepthPyramid.Image, &mapData->NativeGPUData.DepthPyramid.Allocation, nullptr));

			VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;

			// build a image-view for the image
			VkImageViewCreateInfo view_info = VulkanTools::ImageviewCreateInfo(VK_FORMAT_R32_SFLOAT, mapData->NativeGPUData.DepthPyramid.Image, aspectFlag);
			view_info.subresourceRange.levelCount = numPyramidMips;

			VK_CHECK(vkCreateImageView(m_Device.Device, &view_info, nullptr, &mapData->NativeGPUData.DepthPyramid.View));

			// build image views for pyramid
			for (uint32_t i = 0; i < numPyramidMips; i++) {

				VkImageViewCreateInfo view_info = VulkanTools::ImageviewCreateInfo(VK_FORMAT_R32_SFLOAT, mapData->NativeGPUData.DepthPyramid.Image, aspectFlag);
				view_info.subresourceRange.levelCount = 1;
				view_info.subresourceRange.baseMipLevel = i;

				VK_CHECK(vkCreateImageView(m_Device.Device, &view_info, nullptr, &mapData->NativeGPUData.DepthPyramid.ViewMips[i]));
			}
		}

		return mapData;
	}

	void VulkanGfxDevice::DestroyDepthMapGPUData(ResourceGPUData* data) {

		GetCurrentFrame().ExecutionQueue.PushFunction([this, data]() {

			VulkanDepthMapNativeData* nativeData = (VulkanDepthMapNativeData*)data->GetNativeData();

			if (nativeData->View) {

				vkDestroyImageView(m_Device.Device, nativeData->View, nullptr);
			}

			if (nativeData->Image) {

				vmaDestroyImage(m_Device.Allocator, nativeData->Image, nativeData->Allocation);
			}

			if (nativeData->DepthPyramid.View) {

				vkDestroyImageView(m_Device.Device, nativeData->DepthPyramid.View, nullptr);
			}

			for (int i = 0; i < 16; i++) {

				if (nativeData->DepthPyramid.ViewMips[i]) {

					vkDestroyImageView(m_Device.Device, nativeData->DepthPyramid.ViewMips[i], nullptr);
				}
			}

			if (nativeData->DepthPyramid.Image) {

				vmaDestroyImage(m_Device.Allocator, nativeData->DepthPyramid.Image, nativeData->DepthPyramid.Allocation);
			}

			delete data;
			});
	}

	void VulkanGfxDevice::SetMaterialScalarParameter(MaterialResource* material, uint8_t dataBindIndex, float newValue) {

		GetCurrentFrame().ExecutionQueue.PushFunction([=, this]() {

			VulkanMaterialNativeData* nativeData = (VulkanMaterialNativeData*)material->GetGPUData()->GetNativeData();

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
			constants[nativeData->BufferIndex].ScalarData[dataBindIndex] = newValue;

			m_MaterialManager.WriteBuffer(&m_Device, nativeData->BufferIndex);
			});
	}

	void VulkanGfxDevice::SetMaterialColorParameter(MaterialResource* material, uint8_t dataBindIndex, Vector4 newValue) {

		GetCurrentFrame().ExecutionQueue.PushFunction([=, this]() {

			VulkanMaterialNativeData* nativeData = (VulkanMaterialNativeData*)material->GetGPUData()->GetNativeData();

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
			constants[nativeData->BufferIndex].ColorData[dataBindIndex] = glm::vec4(newValue.x, newValue.y, newValue.z, newValue.w);

			m_MaterialManager.WriteBuffer(&m_Device, nativeData->BufferIndex);
			});
	}

	void VulkanGfxDevice::SetMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex, Texture2DResource* newValue) {

		GetCurrentFrame().ExecutionQueue.PushFunction([=, this]() {

			VulkanMaterialNativeData* nativeData = (VulkanMaterialNativeData*)material->GetGPUData()->GetNativeData();

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
			uint32_t texIndex = constants[nativeData->BufferIndex].TexIndex[dataBindIndex];

			VulkanTextureNativeData* texData = (VulkanTextureNativeData*)newValue->GetGPUData()->GetNativeData();
			m_MaterialManager.WriteTexture(&m_Device, texIndex, texData, m_DefSamplerLinear);
			});
	}

	void VulkanGfxDevice::AddMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex) {

		GetCurrentFrame().ExecutionQueue.PushFunction([=, this]() {

			VulkanMaterialNativeData* nativeData = (VulkanMaterialNativeData*)material->GetGPUData()->GetNativeData();

			uint32_t newTexIndex = m_MaterialManager.GetFreeTextureIndex();

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
			constants[nativeData->BufferIndex].TexIndex[dataBindIndex] = newTexIndex;

			m_MaterialManager.WriteBuffer(&m_Device, nativeData->BufferIndex);
			});
	}

	void VulkanGfxDevice::RemoveMaterialTextureParameter(MaterialResource* material, uint8_t dataBindIndex) {

		GetCurrentFrame().ExecutionQueue.PushFunction([=, this]() {

			VulkanMaterialNativeData* nativeData = (VulkanMaterialNativeData*)material->GetGPUData()->GetNativeData();

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)m_MaterialManager.DataBuffer.AllocationInfo.pMappedData;
			m_MaterialManager.ReleaseTextureIndex(constants[nativeData->BufferIndex].TexIndex[dataBindIndex]);

			constants[nativeData->BufferIndex].TexIndex[dataBindIndex] = VulkanMaterialManager::IndexInvalid;

			m_MaterialManager.WriteBuffer(&m_Device, nativeData->BufferIndex);
			});
	}

	void VulkanImGuiTextureManager::Init(uint32_t numDynamicTextures) {

		for (int f = 0; f < FRAME_OVERLAP; f++) {

			DynamicTexSets[f].reserve(numDynamicTextures);

			for (uint32_t t = 0; t < numDynamicTextures; t++) {

				VkDescriptorSet newSet = ImGui_ImplVulkan_CreateTextureDescSet();
				DynamicTexSets[f].push_back(newSet);
			}
		}
	}

	void VulkanImGuiTextureManager::Cleanup() {

		for (int f = 0; f < FRAME_OVERLAP; f++) {

			for (int i = 0; i < DynamicTexSets[f].size(); i++) {

				ImGui_ImplVulkan_RemoveTexture(DynamicTexSets[f][i]);
			}

			DynamicTexSets[f].clear();
		}
	}

	ImTextureID VulkanImGuiTextureManager::CreateStaticGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler) {

		VkDescriptorSet texSet = ImGui_ImplVulkan_AddTexture(sampler, view, layout);
		return (ImTextureID)texSet;
	}

	void VulkanImGuiTextureManager::DestroyStaticGuiTexture(ImTextureID id) {

		VkDescriptorSet texSet = (VkDescriptorSet)id;
		ImGui_ImplVulkan_RemoveTexture(texSet);
	}

	ImTextureID VulkanImGuiTextureManager::CreateDynamicGuiTexture(VkImageView view, VkImageLayout layout, VkSampler sampler, uint32_t currentFrameIndex) {

		VkDescriptorSet dynamicTexSet = DynamicTexSets[currentFrameIndex][DynamicTexSetsIndex];
		DynamicTexSetsIndex++;

		ImGui_ImplVulkan_WriteTextureDescSet(sampler, view, layout, dynamicTexSet);

		return (ImTextureID)dynamicTexSet;
	}

	ImTextureID VulkanGfxDevice::CreateStaticGuiTexture(Texture2DResource* texture) {

		VulkanTextureNativeData* nativeData = (VulkanTextureNativeData*)texture->GetGPUData()->GetNativeData();

		return m_GuiTextureManager.CreateStaticGuiTexture(nativeData->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_DefSamplerLinear);
	}

	void VulkanGfxDevice::DestroyStaticGuiTexture(ImTextureID id) {

		m_GuiTextureManager.DestroyStaticGuiTexture(id);
	}

	ImTextureID VulkanGfxDevice::CreateDynamicGuiTexture(Texture2DResource* texture) {

		VulkanTextureNativeData* nativeData = (VulkanTextureNativeData*)texture->GetGPUData()->GetNativeData();

		uint32_t frameIndex = m_FrameNumber % FRAME_OVERLAP;
		return m_GuiTextureManager.CreateDynamicGuiTexture(nativeData->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_DefSamplerLinear, frameIndex);
	}

	void VulkanGfxDevice::DrawSceneIndirect(SceneRenderProxy* scene, RenderContext renderContext) {

		FrameData& frame = GetCurrentFrame();

		VkDescriptorSet geometrySet = frame.FrameAllocator.Allocate(m_Device.Device, m_GeometrySetLayout);
		VkDescriptorSet lightingSet = frame.FrameAllocator.Allocate(m_Device.Device, m_LightingSetLayout);
		//ENGINE_WARN("Frame set: {}", (void*)frameSet);

		ENGINE_WARN("CameraPos: {0}, {1}, {2}", scene->DrawData.CameraPos.x, scene->DrawData.CameraPos.y, scene->DrawData.CameraPos.z);

		// write scene data buffer
		SceneDrawData* sceneData = (SceneDrawData*)frame.SceneDataBuffer.AllocationInfo.pMappedData;
		sceneData[frame.SceneDataBufferOffset] = scene->DrawData;

		// render passes
		GBufferPass(scene, renderContext, geometrySet);
		LightingPass(scene, renderContext, lightingSet);
		SkyboxPass(renderContext, lightingSet);

		// update offsets
		frame.SceneDataBufferOffset++;
		frame.SceneLightFirstIndex += (uint32_t)scene->Lights.size();
		frame.SceneDrawCommandFirstIndex += (uint32_t)scene->Objects.size();
		frame.SceneDrawCountFirstIndex += m_DrawBatchIDCounter;
		frame.SceneObjectFirstIndex += (uint32_t)scene->Objects.size();
		frame.SceneBatchOffsetFirstIndex += m_DrawBatchIDCounter;
		frame.SceneCullDataFirstIndex++;
		frame.SceneVisibilityBufferFirstIndex += (uint32_t)scene->Objects.size();

		// free data
		delete scene;
	}

	uint32_t VulkanGfxDevice::GetDrawBatchID(VulkanMaterialNativeData* material) {

		uint64_t matPointer = (uint64_t)material;

		auto it = m_DrawBatchIDMap.find(matPointer);
		if (it != m_DrawBatchIDMap.end()) {

			return m_DrawBatchIDMap[matPointer];
		}
		else {

			// create new batch

			uint32_t newbatchID = m_DrawBatchIDCounter;
			m_DrawBatchIDMap[matPointer] = newbatchID;
			m_DrawBatchIDCounter++;

			DrawBatch newBatch{ .CommandsOffset = 0, .CommandsCount = 0, .MatNativeData = material };
			m_DrawBatches.push_back(newBatch);

			return newbatchID;
		}
	}

	// helper function
	glm::vec4 normalizePlane(glm::vec4 p)
	{
		return p / glm::length(glm::vec3(p));
	}

	void VulkanGfxDevice::CullPass(SceneRenderProxy* scene, VkDescriptorSet geometrySet, bool prepass) {

		FrameData& frame = GetCurrentFrame();
		VkCommandBuffer cmd = frame.MainCommandBuffer;

		CullPushData pushConstants{};
		pushConstants.MeshCount = (uint32_t)scene->Objects.size();
		pushConstants.SceneDataOffset = frame.SceneDataBufferOffset;
		pushConstants.DrawCountOffset = frame.SceneDrawCountFirstIndex;
		pushConstants.DrawCommandOffset = frame.SceneDrawCommandFirstIndex;
		pushConstants.MetaDataOffset = frame.SceneObjectFirstIndex;
		pushConstants.BatchOffset = frame.SceneBatchOffsetFirstIndex;
		pushConstants.CullDataOffset = frame.SceneCullDataFirstIndex;
		pushConstants.IsPrepass = prepass ? 1 : 0;

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CullPipeline.Pipeline);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CullPipeline.PipelineLayout, 0, 1, &geometrySet, 0, nullptr);
		vkCmdPushConstants(cmd, m_CullPipeline.PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

		uint32_t groupSize = uint32_t((scene->Objects.size() / 256) + 1);

		vkCmdDispatch(cmd, groupSize, 1, 1);

		VulkanTools::TransitionBuffer(cmd, frame.DrawCommandsBuffer.Buffer, scene->Objects.size() * sizeof(DrawIndirectCommand), frame.SceneDrawCommandFirstIndex * sizeof(DrawIndirectCommand),
			VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT);

		VulkanTools::TransitionBuffer(cmd, frame.DrawCountBuffer.Buffer, m_DrawBatchIDCounter * sizeof(uint32_t), frame.SceneDrawCountFirstIndex * sizeof(uint32_t), 
			VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT);
	}

	void VulkanGfxDevice::GeometryDrawPass(SceneRenderProxy* scene, RenderContext context, VkDescriptorSet geometrySet, bool prepass) {

		FrameData& frame = GetCurrentFrame();
		VkCommandBuffer cmd = frame.MainCommandBuffer;
		GBufferResource* gBuffer = context.GBuffer;

		VulkanTextureNativeData* albedoMapNativeData = (VulkanTextureNativeData*)gBuffer->GetAlbedoMapData()->GetNativeData();
		VulkanTextureNativeData* normalMapNativeData = (VulkanTextureNativeData*)gBuffer->GetNormalMapData()->GetNativeData();
		VulkanTextureNativeData* materialMapNativeData = (VulkanTextureNativeData*)gBuffer->GetMaterialMapData()->GetNativeData();
		VulkanDepthMapNativeData* depthNativeData = (VulkanDepthMapNativeData*)gBuffer->GetDepthMapData()->GetNativeData();

		VkClearValue depthClear = { .depthStencil = {0.0f, 0} };
		VkClearValue colorClear = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };

		VkRenderingAttachmentInfo depthAttachment = VulkanTools::DepthAttachmentInfo(depthNativeData->View, prepass ? &depthClear : nullptr, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		std::array<VkRenderingAttachmentInfo, 3> colorAttachments{};
		colorAttachments[0] = VulkanTools::AttachmentInfo(albedoMapNativeData->View, prepass ? &colorClear : nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		colorAttachments[1] = VulkanTools::AttachmentInfo(normalMapNativeData->View, prepass ? &colorClear : nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		colorAttachments[2] = VulkanTools::AttachmentInfo(materialMapNativeData->View, prepass ? &colorClear : nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkExtent2D drawExtent = { gBuffer->GetWidth(), gBuffer->GetHeight() };

		VulkanMaterialManager::MaterialPushConstants pushConstants{};
		pushConstants.SceneDataOffset = frame.SceneDataBufferOffset;

		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(drawExtent, colorAttachments.data(), 3, &depthAttachment);
		vkCmdBeginRendering(cmd, &renderInfo);

		for (int i = 0; i < m_DrawBatches.size(); i++) {

			DrawBatch& currentBatch = m_DrawBatches[i];
			VulkanMaterialNativeData* matData = currentBatch.MatNativeData;

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->Pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->PipelineLayout, 0, 1, &geometrySet, 0, nullptr);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->PipelineLayout, 1, 1, &m_MaterialManager.DataSet, 0, nullptr);

			vkCmdPushConstants(cmd, matData->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)drawExtent.width;
			viewport.height = (float)drawExtent.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = drawExtent.width;
			scissor.extent.height = drawExtent.height;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			uint32_t stride = sizeof(DrawIndirectCommand);
			uint64_t commandOffset = frame.SceneDrawCommandFirstIndex + currentBatch.CommandsOffset;
			uint64_t countOffset = frame.SceneDrawCountFirstIndex + i;

			vkCmdDrawIndirectCount(cmd, frame.DrawCommandsBuffer.Buffer, commandOffset * stride, frame.DrawCountBuffer.Buffer, countOffset * sizeof(uint32_t), (uint32_t)scene->Objects.size(), stride);
		}

		vkCmdEndRendering(cmd);
	}

	void VulkanGfxDevice::GBufferPass(SceneRenderProxy* scene, RenderContext context, VkDescriptorSet geometrySet) {

		FrameData& frame = GetCurrentFrame();
		FrameData& lastFrame = GetLastFrame();

		// write buffer with metadata
		{
			SceneObjectMetadata* metaBuffer = (SceneObjectMetadata*)frame.SceneObjectMetaBuffer.AllocationInfo.pMappedData;

			m_DrawBatches.clear();
			m_DrawBatchIDMap.clear();
			m_DrawBatchIDCounter = 0;

			Timer cpuTimer;

			for (int i = 0; i < scene->Objects.size(); i++) {

				SceneObjectProxy& object = scene->Objects[i];

				VulkanMeshNativeData* meshNativeData = (VulkanMeshNativeData*)object.Mesh->GetGPUData()->GetNativeData();
				VulkanMaterialNativeData* matNativeData = (VulkanMaterialNativeData*)object.Material->GetGPUData()->GetNativeData();

				uint32_t batchID = GetDrawBatchID(matNativeData);
				m_DrawBatches[batchID].CommandsCount++;

				//object.LastVisibilityIndex = object.CurrentVisibilityIndex;
				//object.CurrentVisibilityIndex = m_CullPipeline.SceneVisibilityBufferFirstIndex + i;

				metaBuffer[frame.SceneObjectFirstIndex + i] = {

					.BoundsOrigin = glm::vec4(object.BoundsOrigin.x, object.BoundsOrigin.y, object.BoundsOrigin.z, object.BoundsRadius),
					.BoundsExtents = glm::vec4(object.BoundsExtents.x, object.BoundsExtents.y, object.BoundsExtents.z, 0),
					.GlobalTransform = object.GlobalTransform,
					.FirstIndex = object.FirstIndex,
					.IndexCount = object.IndexCount,
					.IndexBufferAddress = meshNativeData->IndexBufferAddress,
					.VertexBufferAddress = meshNativeData->VertexBufferAddress,
					.MaterialBufferIndex = matNativeData->BufferIndex,
					.DrawBatchID = batchID,
					.LastVisibilityIndex = object.LastVisibilityIndex,
					.CurrentVisibilityIndex = object.CurrentVisibilityIndex
				};
			}

			ENGINE_INFO("Waited for objects: {0} ms, {1} num of objects.", cpuTimer.GetElapsedMs(), scene->Objects.size());
		}

		// compute batch offsets
		{
			uint32_t* batchOffsets = (uint32_t*)frame.BatchOffsetsBuffer.AllocationInfo.pMappedData;

			for (int i = 0; i < m_DrawBatches.size(); i++) {

				if (i > 0) {

					m_DrawBatches[i].CommandsOffset = m_DrawBatches[i - 1].CommandsOffset + m_DrawBatches[i - 1].CommandsCount;
				}

				batchOffsets[frame.SceneBatchOffsetFirstIndex + i] = m_DrawBatches[i].CommandsOffset;
			}
		}

		GBufferResource* gBuffer = context.GBuffer;
		VulkanDepthMapNativeData* depthNativeData = (VulkanDepthMapNativeData*)gBuffer->GetDepthMapData()->GetNativeData();
		SceneDrawData& sceneData = scene->DrawData;

		VkCommandBuffer cmd = frame.MainCommandBuffer;

		// compute cull data
		{
			CullData cullData{};

			glm::mat4& vp = scene->DrawData.ViewProj;

			cullData.FrustumPlanes[0] = glm::vec4( 
				vp[0][3] + vp[0][0], vp[1][3] + vp[1][0],
				vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
			cullData.FrustumPlanes[1] = glm::vec4(
				vp[0][3] - vp[0][0], vp[1][3] - vp[1][0],
				vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
			cullData.FrustumPlanes[2] = glm::vec4(
				vp[0][3] + vp[0][1], vp[1][3] + vp[1][1],
				vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
			cullData.FrustumPlanes[3] = glm::vec4(
				vp[0][3] - vp[0][1], vp[1][3] - vp[1][1],
				vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
			cullData.FrustumPlanes[4] = glm::vec4(
				vp[0][3] + vp[0][2], vp[1][3] + vp[1][2],
				vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
			cullData.FrustumPlanes[5] = glm::vec4(
				vp[0][3] - vp[0][2], vp[1][3] - vp[1][2],
				vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

			for (int i = 0; i < 6; i++) {

				float length = glm::length(glm::vec3(cullData.FrustumPlanes[i]));
				cullData.FrustumPlanes[i] /= length;
			}

			cullData.P00 = scene->DrawData.Proj[0][0];
			cullData.P11 = scene->DrawData.Proj[1][1];
			cullData.PyramidSize = gBuffer->GetPyramidSize();

			CullData* cullDataBuffer = (CullData*)frame.CullDataBuffer.AllocationInfo.pMappedData;
			cullDataBuffer[frame.SceneCullDataFirstIndex] = cullData;
		}

		// write frame set
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, frame.SceneDataBuffer.Buffer, sizeof(SceneDrawData), frame.SceneDataBufferOffset * sizeof(SceneDrawData), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.WriteBuffer(1, frame.DrawCommandsBuffer.Buffer, scene->Objects.size() * sizeof(DrawIndirectCommand), frame.SceneDrawCommandFirstIndex * sizeof(DrawIndirectCommand), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.WriteBuffer(2, frame.DrawCountBuffer.Buffer, m_DrawBatchIDCounter * sizeof(uint32_t), frame.SceneDrawCountFirstIndex * sizeof(uint32_t), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.WriteBuffer(3, frame.SceneObjectMetaBuffer.Buffer, scene->Objects.size() * sizeof(SceneObjectMetadata), frame.SceneObjectFirstIndex * sizeof(SceneObjectMetadata), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.WriteBuffer(4, frame.BatchOffsetsBuffer.Buffer, m_DrawBatchIDCounter * sizeof(uint32_t), frame.SceneBatchOffsetFirstIndex * sizeof(uint32_t), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

			VulkanTools::TransitionImage(cmd, depthNativeData->DepthPyramid.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			writer.WriteImage(5, depthNativeData->DepthPyramid.View, m_DepthPyramidPipeline.DepthPyramidSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.WriteBuffer(6, frame.CullDataBuffer.Buffer, sizeof(CullData), frame.SceneCullDataFirstIndex * sizeof(CullData), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.WriteBuffer(7, frame.VisibilityBuffer.Buffer, scene->Objects.size() * sizeof(uint32_t), frame.SceneVisibilityBufferFirstIndex * sizeof(uint32_t), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.WriteBuffer(8, lastFrame.VisibilityBuffer.Buffer, VK_WHOLE_SIZE, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.UpdateSet(m_Device.Device, geometrySet);
		}

		VulkanTextureNativeData* albedoMapNativeData = (VulkanTextureNativeData*)gBuffer->GetAlbedoMapData()->GetNativeData();
		VulkanTextureNativeData* normalMapNativeData = (VulkanTextureNativeData*)gBuffer->GetNormalMapData()->GetNativeData();
		VulkanTextureNativeData* materialMapNativeData = (VulkanTextureNativeData*)gBuffer->GetMaterialMapData()->GetNativeData();

		// first cull pass
		{
			CullPass(scene, geometrySet, true);

			VulkanTools::TransitionImage(cmd, albedoMapNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			VulkanTools::TransitionImage(cmd, normalMapNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			VulkanTools::TransitionImage(cmd, materialMapNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			VulkanTools::TransitionImage(cmd, depthNativeData->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			GeometryDrawPass(scene, context, geometrySet, true);
		}

		// generate depth pyramid
		{
			VulkanTools::TransitionImage(cmd, depthNativeData->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VulkanTools::TransitionImage(cmd, depthNativeData->DepthPyramid.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_DepthPyramidPipeline.Pipeline);

			//uint32_t numMips = uint32_t(std::floor(std::log2(gBuffer->GetPyramidSize()))) + 1;
			uint32_t numMips = uint32_t(glm::log2(float(gBuffer->GetPyramidSize()))) + 1;;
			for (uint32_t i = 0; i < numMips; i++) {

				VkDescriptorSet depthSet = frame.FrameAllocator.Allocate(m_Device.Device, m_DepthPyramidPipeline.SetLayout);

				// write set
				{
					DescriptorWriter writer;
					writer.WriteImage(0, depthNativeData->DepthPyramid.ViewMips[i], m_DepthPyramidPipeline.DepthPyramidSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

					if (i == 0) {

						writer.WriteImage(1, depthNativeData->View, m_DepthPyramidPipeline.DepthPyramidSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
					}
					else {

						writer.WriteImage(1, depthNativeData->DepthPyramid.ViewMips[i - 1], m_DepthPyramidPipeline.DepthPyramidSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
					}

					writer.UpdateSet(m_Device.Device, depthSet);
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_DepthPyramidPipeline.PipelineLayout, 0, 1, &depthSet, 0, nullptr);

				uint32_t levelSize = gBuffer->GetPyramidSize() >> i;
				//if (levelSize < 1) levelSize = 1;

				DepthReduceData reduceData = { .PyramidSize = levelSize };

				// execute the shader
				vkCmdPushConstants(cmd, m_DepthPyramidPipeline.PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(reduceData), &reduceData);
				vkCmdDispatch(cmd, VulkanTools::GetComputeGroupCount(levelSize, 32), VulkanTools::GetComputeGroupCount(levelSize, 32), 1);

				VulkanTools::TransitionImage(cmd, depthNativeData->DepthPyramid.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT);
			}

			VulkanTools::TransitionImage(cmd, depthNativeData->DepthPyramid.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		// second cull pass
		{
			VulkanTools::FillBuffer(cmd, frame.DrawCountBuffer.Buffer, m_DrawBatchIDCounter * sizeof(uint32_t), frame.SceneDrawCountFirstIndex * sizeof(uint32_t), 0,
				VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

			VulkanTools::TransitionBuffer(cmd, frame.DrawCommandsBuffer.Buffer, scene->Objects.size() * sizeof(DrawIndirectCommand), frame.SceneDrawCommandFirstIndex * sizeof(DrawIndirectCommand),
				VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

			CullPass(scene, geometrySet, false);

			VulkanTools::TransitionImage(cmd, depthNativeData->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			GeometryDrawPass(scene, context, geometrySet, false);
		}
	}

	void VulkanGfxDevice::LightingPass(SceneRenderProxy* scene, RenderContext context, VkDescriptorSet lightingSet) {

		FrameData& frame = GetCurrentFrame();
		
		// write lights buffer
		{
			SceneLightProxy* lightsBuffer = (SceneLightProxy*)frame.LightsBuffer.AllocationInfo.pMappedData;

			for (int i = 0; i < scene->Lights.size(); i++) {

				SceneLightProxy& light = scene->Lights[i];

				lightsBuffer[frame.SceneLightFirstIndex + i] = light;
			}
		}

		GBufferResource* gBuffer = context.GBuffer;
		VkCommandBuffer cmd = frame.MainCommandBuffer;

		VulkanTextureNativeData* albedoMapNativeData = (VulkanTextureNativeData*)gBuffer->GetAlbedoMapData()->GetNativeData();
		VulkanTextureNativeData* normalMapNativeData = (VulkanTextureNativeData*)gBuffer->GetNormalMapData()->GetNativeData();
		VulkanTextureNativeData* materialMapNativeData = (VulkanTextureNativeData*)gBuffer->GetMaterialMapData()->GetNativeData();
		VulkanDepthMapNativeData* depthNativeData = (VulkanDepthMapNativeData*)gBuffer->GetDepthMapData()->GetNativeData();

		VulkanCubeTextureNativeData* envMapNativeData = (VulkanCubeTextureNativeData*)context.EnvironmentTexture->GetGPUData()->GetNativeData();
		VulkanCubeTextureNativeData* irradiaceMapNativeData = (VulkanCubeTextureNativeData*)context.IrradianceTexture->GetGPUData()->GetNativeData();
		VulkanTextureNativeData* outTextureNativeData = (VulkanTextureNativeData*)context.OutTexture->GetGPUData()->GetNativeData();

		// transition images
		{
			VulkanTools::TransitionImage(cmd, albedoMapNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VulkanTools::TransitionImage(cmd, normalMapNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VulkanTools::TransitionImage(cmd, materialMapNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VulkanTools::TransitionImage(cmd, depthNativeData->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		// write desc set
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, frame.SceneDataBuffer.Buffer, sizeof(SceneDrawData), frame.SceneDataBufferOffset * sizeof(SceneDrawData), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.WriteImage(1, albedoMapNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(2, normalMapNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(3, materialMapNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(4, depthNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(5, envMapNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteBuffer(6, frame.LightsBuffer.Buffer, scene->Lights.size() * sizeof(SceneLightProxy), frame.SceneLightFirstIndex * sizeof(SceneLightProxy), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.WriteImage(7, irradiaceMapNativeData->View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(8, m_BRDFLutTexture->NativeGPUData.View, m_DefSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.UpdateSet(m_Device.Device, lightingSet);
		}

		// render frame
		{
			VulkanTools::TransitionImage(cmd, outTextureNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkExtent2D drawExtent = {

				context.OutTexture->GetWidth(),
				context.OutTexture->GetHeight()
			};

			VkClearValue colorClear = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
			VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(outTextureNativeData->View, &colorClear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(drawExtent, &colorAttachment, 1, VK_NULL_HANDLE);
			vkCmdBeginRendering(cmd, &renderInfo);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline.Pipeline);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline.PipelineLayout, 0, 1, &lightingSet, 0, nullptr);

			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)drawExtent.width;
			viewport.height = (float)drawExtent.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = drawExtent.width;
			scissor.extent.height = drawExtent.height;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			LightingData pushConstants{};
			pushConstants.SceneDataOffset = frame.SceneDataBufferOffset;
			pushConstants.LightsOffset = frame.SceneLightFirstIndex;
			pushConstants.LightsCount = (uint32_t)scene->Lights.size();
			pushConstants.EnvironmentMapNumMips = uint32_t(std::floor(std::log2(context.EnvironmentTexture->GetSize()))) + 1;

			vkCmdPushConstants(cmd, m_LightingPipeline.PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

			// draw fullscreen quad
			vkCmdDraw(cmd, 6, 1, 0, 0);

			vkCmdEndRendering(cmd);
		}
	}

	void VulkanGfxDevice::SkyboxPass(RenderContext context, VkDescriptorSet lightingSet) {

		VulkanTextureNativeData* outTextureNativeData = (VulkanTextureNativeData*)context.OutTexture->GetGPUData()->GetNativeData();
		VulkanTextureNativeData* depthNativeData = (VulkanTextureNativeData*)context.GBuffer->GetDepthMapData()->GetNativeData();

		FrameData& frame = GetCurrentFrame();
		VkCommandBuffer cmd = frame.MainCommandBuffer;

		VulkanTools::TransitionImage(cmd, depthNativeData->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		VulkanTools::TransitionImage(cmd, outTextureNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// render skybox
		{
			VkExtent2D drawExtent = {

				context.OutTexture->GetWidth(),
				context.OutTexture->GetHeight()
			};

			VkRenderingAttachmentInfo colorAttachment = VulkanTools::AttachmentInfo(outTextureNativeData->View, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			VkRenderingAttachmentInfo depthAttachment = VulkanTools::DepthAttachmentInfo(depthNativeData->View, nullptr, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(drawExtent, &colorAttachment, 1, &depthAttachment);
			vkCmdBeginRendering(cmd, &renderInfo);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipeline.Pipeline);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPipeline.PipelineLayout, 0, 1, &lightingSet, 0, nullptr);

			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)drawExtent.width;
			viewport.height = (float)drawExtent.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = drawExtent.width;
			scissor.extent.height = drawExtent.height;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			SkyboxData pushConstants{};
			pushConstants.SceneDataOffset = frame.SceneDataBufferOffset;

			vkCmdPushConstants(cmd, m_SkyboxPipeline.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

			vkCmdDraw(cmd, 3, 1, 0, 0);

			vkCmdEndRendering(cmd);
		}

		VulkanTools::TransitionImage(cmd, outTextureNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VulkanTools::TransitionImage(cmd, depthNativeData->Image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanGfxDevice::BeginFrameCommandRecording() {

		//ENGINE_WARN("Frame begin");

		FrameData& frame = GetCurrentFrame();
		FrameData& lastFrame = GetLastFrame();

		VkCommandBuffer cmd = frame.MainCommandBuffer;

		Timer gpuTimer;
		VK_CHECK(vkWaitForFences(m_Device.Device, 1, &frame.RenderFence, true, 1000000000));

		frame.ExecutionQueue.Flush();
		frame.FrameAllocator.ClearDescriptors(m_Device.Device);

		ENGINE_INFO("Waited for gpu: {0} ms", gpuTimer.GetElapsedMs());

		VK_CHECK(vkResetFences(m_Device.Device, 1, &frame.RenderFence));
		VK_CHECK(vkResetCommandBuffer(cmd, 0));

		frame.SceneDataBufferOffset = 0;
		frame.SceneLightFirstIndex = 0;
		frame.SceneDrawCommandFirstIndex = 0;
		frame.SceneDrawCountFirstIndex = 0;
		frame.SceneObjectFirstIndex = 0;
		frame.SceneBatchOffsetFirstIndex = 0;
		frame.SceneCullDataFirstIndex = 0;
		frame.SceneVisibilityBufferFirstIndex = 0;

		m_GuiTextureManager.DynamicTexSetsIndex = 0;

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanTools::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		VulkanTools::FillBuffer(cmd, frame.DrawCountBuffer.Buffer, VK_WHOLE_SIZE, 0, 0, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

		// barrier last frame visibility buffer cause we need it in current frame draws
		VulkanTools::TransitionBuffer(cmd, lastFrame.VisibilityBuffer.Buffer, VK_WHOLE_SIZE, 0, VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

		// imgui
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			static bool dockOpen = true;
			static bool opt_fullscreen = true;
			static bool opt_padding = false;
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			if (opt_fullscreen)
			{
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			}
			else
			{
				dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
			}

			if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				window_flags |= ImGuiWindowFlags_NoBackground;

			if (!opt_padding)
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace Demo", &dockOpen, window_flags);
			if (!opt_padding)
				ImGui::PopStyleVar();

			if (opt_fullscreen)
				ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}
		}
	}

	void VulkanGfxDevice::DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView, bool clearSwaphain) {

		ImGui::End();

		ImGui::Render();

		VkRenderingAttachmentInfo colorAttachment{};
		if (clearSwaphain) {

			VkClearValue colorClear = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
			colorAttachment = VulkanTools::AttachmentInfo(targetImageView, &colorClear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		} else {
			colorAttachment = VulkanTools::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		VkRenderingInfo renderInfo = VulkanTools::RenderingInfo(m_Swapchain.Extent, &colorAttachment, 1, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	void VulkanGfxDevice::DrawSwapchain(uint32_t width, uint32_t height, bool drawGui, Texture2DResource* fillTexture) {

		FrameData& frame = GetCurrentFrame();
		VkCommandBuffer cmd = frame.MainCommandBuffer;

		uint32_t swapchainImageIndex = 0;

		VkResult check = vkAcquireNextImageKHR(m_Device.Device, m_Swapchain.Swapchain, 1000000000, frame.SwapchainSemaphore, nullptr, &swapchainImageIndex);
		if (check == VK_ERROR_OUT_OF_DATE_KHR) {

			// resize swapchain
			m_Swapchain.Resize(&m_Device, width, height);
			VK_CHECK(vkAcquireNextImageKHR(m_Device.Device, m_Swapchain.Swapchain, 1000000000, frame.SwapchainSemaphore, nullptr, &swapchainImageIndex));
		}

		if (fillTexture) {

			VulkanTextureNativeData* fillNativeData = (VulkanTextureNativeData*)fillTexture->GetGPUData()->GetNativeData();

			VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			VulkanTools::TransitionImage(cmd, fillNativeData->Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VulkanTools::CopyImageToImage(cmd, fillNativeData->Image, m_Swapchain.Images[swapchainImageIndex], { fillTexture->GetWidth(), fillTexture->GetHeight() }, m_Swapchain.Extent);

			if (drawGui) {

				VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				DrawImGui(cmd, m_Swapchain.ImageViews[swapchainImageIndex], false);
				VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}
			else {
				VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}
		} 
		else if (drawGui) {

			VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			DrawImGui(cmd, m_Swapchain.ImageViews[swapchainImageIndex], true);
			VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}
		else {

			ENGINE_WARN("Not rendering anything to swapchain at frame: {0}! This should not happen.", m_FrameNumber);

			VulkanTools::TransitionImage(cmd, m_Swapchain.Images[swapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}

		// submit all commands from current frame
		{
			VK_CHECK(vkEndCommandBuffer(cmd));

			VkCommandBufferSubmitInfo cmdInfo = VulkanTools::CommandBufferSubmitInfo(cmd);

			VkSemaphoreSubmitInfo waitInfo = VulkanTools::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.SwapchainSemaphore);
			VkSemaphoreSubmitInfo signalInfo = VulkanTools::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.RenderSemaphore);

			VkSubmitInfo2 submit = VulkanTools::SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

			VK_CHECK(vkQueueSubmit2(m_Device.Queues.GraphicsQueue, 1, &submit, frame.RenderFence));

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.pSwapchains = &m_Swapchain.Swapchain;
			presentInfo.swapchainCount = 1;

			presentInfo.pWaitSemaphores = &frame.RenderSemaphore;
			presentInfo.waitSemaphoreCount = 1;

			presentInfo.pImageIndices = &swapchainImageIndex;

			VK_CHECK(vkQueuePresentKHR(m_Device.Queues.GraphicsQueue, &presentInfo));
		}

		m_FrameNumber++;
	}
}