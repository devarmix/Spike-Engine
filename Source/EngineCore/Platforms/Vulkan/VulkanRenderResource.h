#pragma once

#include <Engine/Renderer/RenderResource.h>
#include <Platforms/Vulkan/VulkanCommon.h>
#include <Platforms/Vulkan/VulkanBuffer.h>

namespace Spike {

	struct VulkanTextureNativeData {

		VkImage Image;
		VkImageView View;
		VmaAllocation Allocation;
	};

	class VulkanTexture2DGPUData : public ResourceGPUData {
	public:

		VulkanTexture2DGPUData() 
			: NativeGPUData{.Image = nullptr, .View = nullptr, .Allocation = nullptr} {}

		virtual ~VulkanTexture2DGPUData() override {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanTextureNativeData NativeGPUData;
	};


	struct VulkanDepthMapNativeData {

		VkImage Image;
		VkImageView View;
		VmaAllocation Allocation;

		struct {

			VkImage Image;
			VkImageView View;
			VkImageView ViewMips[16];
			VmaAllocation Allocation;

		} DepthPyramid;
	};

	class VulkanDepthMapGPUData : public ResourceGPUData {
	public:

		VulkanDepthMapGPUData() 
			: NativeGPUData{ .Image = nullptr, .View = nullptr, .Allocation = nullptr, 
			.DepthPyramid{.Image = nullptr, .Allocation = nullptr} } 
		{
			for (int i = 0; i < 16; i++) {

				NativeGPUData.DepthPyramid.ViewMips[i] = nullptr;
			}

		}

		virtual ~VulkanDepthMapGPUData() override {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanDepthMapNativeData NativeGPUData;
	};


	struct VulkanCubeTextureNativeData {

		VkImage Image;
		VkImageView View;
		VmaAllocation Allocation;
	};

	class VulkanCubeTextureGPUData : public ResourceGPUData {
	public:

		VulkanCubeTextureGPUData() 
			: NativeGPUData{.Image = nullptr, .View = nullptr, .Allocation = nullptr} {}

		virtual ~VulkanCubeTextureGPUData() override {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanCubeTextureNativeData NativeGPUData;
	};


	struct VulkanMaterialNativeData {

		uint32_t BufferIndex;

		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

	class VulkanMaterialGPUData : public ResourceGPUData {
	public:

		VulkanMaterialGPUData() 
			: NativeGPUData{.BufferIndex = 0, .Pipeline = nullptr, .PipelineLayout = nullptr}  {}

		virtual ~VulkanMaterialGPUData() override {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanMaterialNativeData NativeGPUData;
	};


	struct VulkanMeshNativeData {

		VulkanBuffer IndexBuffer;
		VulkanBuffer VertexBuffer;

		VkDeviceAddress VertexBufferAddress;
		VkDeviceAddress IndexBufferAddress;
	};

	class VulkanMeshGPUData : public ResourceGPUData {
	public:

		VulkanMeshGPUData() 
			: NativeGPUData{.VertexBufferAddress = 0, .IndexBufferAddress = 0} {}

		virtual ~VulkanMeshGPUData() override {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanMeshNativeData NativeGPUData;
	};
}