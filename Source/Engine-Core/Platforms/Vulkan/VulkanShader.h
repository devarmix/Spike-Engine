#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Engine/Core/Core.h>

namespace Spike {

	class VulkanShader {
	public:

		VulkanShader() = default;

		void Destroy();

		static VulkanShader Create(const char* vertexPath, const char* fragmentPath);

	public:

		VkShaderModule VertexModule;
		VkShaderModule FragmentModule;
	};
}
