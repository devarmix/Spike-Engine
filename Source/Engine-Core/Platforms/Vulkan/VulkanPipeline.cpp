#include <Platforms/Vulkan/VulkanPipeline.h>
#include <Platforms/Vulkan/VulkanTools.h>
#include <Platforms/Vulkan/VulkanDescriptors.h>

#include <Platforms/Vulkan/VulkanRenderer.h>

namespace Spike {

	void PipelineBuilder::Clear() {

		InputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		Rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		ColorBlendAttachment = {};
		Multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		PipelineLayout = {};
		DepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		RenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		ShaderStages.clear();
	}

	VkPipeline PipelineBuilder::BuildPipeline(VkDevice device) {

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;

		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
		ColorBlendAttachments.reserve(RenderInfo.colorAttachmentCount);

		for (uint32_t i = 0; i < RenderInfo.colorAttachmentCount; i++) {

			ColorBlendAttachments.push_back(ColorBlendAttachment);
		}

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = (uint32_t)ColorBlendAttachments.size();
		colorBlending.pAttachments = ColorBlendAttachments.data();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.pNext = &RenderInfo;

		pipelineInfo.stageCount = (uint32_t)ShaderStages.size();
		pipelineInfo.pStages = ShaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &InputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &Rasterizer;
		pipelineInfo.pMultisampleState = &Multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &DepthStencil;
		pipelineInfo.layout = PipelineLayout;

		VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicInfo.pDynamicStates = &state[0];
		dynamicInfo.dynamicStateCount = 2;

		pipelineInfo.pDynamicState = &dynamicInfo;

		VkPipeline newPipeline;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {

			ENGINE_ERROR("Failed To Create Pipeline!");
			return VK_NULL_HANDLE; // failed to create graphics pipeline
		}
		else {
			return newPipeline;
		}
	}

	void PipelineBuilder::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {

		ShaderStages.clear();

		ShaderStages.push_back(
			VulkanTools::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

		ShaderStages.push_back(
			VulkanTools::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
	}

	void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology) {

		InputAssembly.topology = topology;
		InputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	void PipelineBuilder::SetPolygonMode(VkPolygonMode mode) {

		Rasterizer.polygonMode = mode;
		Rasterizer.lineWidth = 1.f;
	}

	void PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {

		Rasterizer.cullMode = cullMode;
		Rasterizer.frontFace = frontFace;
	}

	void PipelineBuilder::SetMultisamplingNone() {

		Multisampling.sampleShadingEnable = VK_FALSE;

		Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		Multisampling.minSampleShading = 1.0f;
		Multisampling.pSampleMask = nullptr;

		Multisampling.alphaToCoverageEnable = VK_FALSE;
		Multisampling.alphaToOneEnable = VK_FALSE;
	}

	void PipelineBuilder::DisableBlending() {

		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		ColorBlendAttachment.blendEnable = VK_FALSE;
	}

	void PipelineBuilder::SetColorAttachments(VkFormat* formats, uint32_t count) {

		RenderInfo.colorAttachmentCount = count;
		RenderInfo.pColorAttachmentFormats = formats;
	}

	void PipelineBuilder::SetDepthFormat(VkFormat format) {

		RenderInfo.depthAttachmentFormat = format;
	}

	void PipelineBuilder::DisableDepthTest() {

		DepthStencil.depthTestEnable = VK_FALSE;
		DepthStencil.depthWriteEnable = VK_FALSE;
		DepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
		DepthStencil.depthBoundsTestEnable = VK_FALSE;
		DepthStencil.stencilTestEnable = VK_FALSE;
		DepthStencil.front = {};
		DepthStencil.back = {};
		DepthStencil.minDepthBounds = 0.f;
		DepthStencil.maxDepthBounds = 1.f;
	}

	void PipelineBuilder::EnableDepthTest(bool depthWriteEnable, VkCompareOp op) {

		DepthStencil.depthTestEnable = VK_TRUE;
		DepthStencil.depthWriteEnable = depthWriteEnable;
		DepthStencil.depthCompareOp = op;
		DepthStencil.depthBoundsTestEnable = VK_FALSE;
		DepthStencil.stencilTestEnable = VK_FALSE;
		DepthStencil.front = {};
		DepthStencil.back = {};
		DepthStencil.minDepthBounds = 0.f;
		DepthStencil.maxDepthBounds = 1.f;
	}

	void PipelineBuilder::EnableBlendingAdditive() {

		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachment.blendEnable = VK_TRUE;
		ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PipelineBuilder::EnableBlendingAlphablend() {

		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachment.blendEnable = VK_TRUE;
		ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
}