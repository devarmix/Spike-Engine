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

		VulkanTextureNativeData Depth;

		VulkanTextureNativeData Pyramid;
		VkImageView PyramidViews[16];
	};

	class VulkanDepthMapGPUData : public ResourceGPUData {
	public:

		VulkanDepthMapGPUData() 
			: NativeGPUData{.Depth = {nullptr, nullptr, nullptr}, 
			.Pyramid = {nullptr, nullptr, nullptr}}
		{
			for (int i = 0; i < 16; i++) {

				NativeGPUData.PyramidViews[i] = nullptr;
			}

		}

		virtual ~VulkanDepthMapGPUData() override {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanDepthMapNativeData NativeGPUData;
	};


	struct VulkanBloomMapNativeData {

		VulkanTextureNativeData BloomComposite;
		VulkanTextureNativeData BloomDownSample;
		VulkanTextureNativeData BloomUpSample;

		VkImageView DownSampleViews[16];
		VkImageView UpSampleViews[16];
	};

	class VulkanBloomMapGPUData : public ResourceGPUData {
	public:

		VulkanBloomMapGPUData() 
			: NativeGPUData{ .BloomComposite = {nullptr, nullptr, nullptr}, 
			.BloomDownSample{nullptr, nullptr, nullptr}, 
			.BloomUpSample{nullptr, nullptr, nullptr} } 
		{
			for (int i = 0; i < 16; i++) {

				NativeGPUData.DownSampleViews[i] = nullptr;
				NativeGPUData.UpSampleViews[i] = nullptr;
			}
		}

		virtual ~VulkanBloomMapGPUData() {}

		virtual void* GetNativeData() override { return (void*)&NativeGPUData; }

	public:

		VulkanBloomMapNativeData NativeGPUData;
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