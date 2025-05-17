#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>

namespace Spike {

	class PipelineBuilder {
	public:
		std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

		VkPipelineInputAssemblyStateCreateInfo InputAssembly;
		VkPipelineRasterizationStateCreateInfo Rasterizer;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo Multisampling;
		VkPipelineLayout PipelineLayout;
		VkPipelineDepthStencilStateCreateInfo DepthStencil;
		VkPipelineRenderingCreateInfo RenderInfo;

		PipelineBuilder() { Clear(); }

		void Clear();

		VkPipeline BuildPipeline(VkDevice device);

		void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
		void SetInputTopology(VkPrimitiveTopology topology);
		void SetPolygonMode(VkPolygonMode mode);
		void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void SetMultisamplingNone();
		void DisableBlending();
		void SetColorAttachments(const VkFormat* formats, uint32_t count);
		void SetDepthFormat(VkFormat format);
		void DisableDepthTest();
		void EnableDepthTest(bool depthWriteEnable, VkCompareOp op);

		void EnableBlendingAdditive();
		void EnableBlendingAlphablend();
	};
}