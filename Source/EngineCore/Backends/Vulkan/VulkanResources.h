#pragma once

#include <Engine/Renderer/TextureBase.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/Shader.h>
#include <Backends/Vulkan/VulkanCommon.h>

namespace Spike {

	class VulkanRHITexture : public RHIData {
	public:
		VulkanRHITexture() {}
		virtual ~VulkanRHITexture() override {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkImage Image = nullptr;
			VmaAllocation Allocation = nullptr;
		} NativeData;
	};

	class VulkanRHITextureView : public RHIData {
	public:
		VulkanRHITextureView() {}
		virtual ~VulkanRHITextureView() {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkImageView View = nullptr;
		} NativeData;
	};

	class VulkanRHISampler : public RHIData {
	public:
		VulkanRHISampler() {}
		virtual ~VulkanRHISampler() {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkSampler Sampler = nullptr;
		} NativeData;
	};

	class VulkanRHIBuffer : public RHIData {
	public:
		VulkanRHIBuffer() {}
		virtual ~VulkanRHIBuffer() {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkBuffer Buffer = nullptr;
			VmaAllocation Allocation = nullptr;
			VmaAllocationInfo AllocationInfo;
		} NativeData;
	};

	class VulkanRHICommandBuffer : public RHIData {
	public:
		VulkanRHICommandBuffer() {}
		virtual ~VulkanRHICommandBuffer() {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkCommandBuffer Cmd = nullptr;
			VkCommandPool Pool = nullptr;
		} NativeData;
	};

	class VulkanRHIShader : public RHIData {
	public:
		VulkanRHIShader() {}
		virtual ~VulkanRHIShader() {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkPipeline Pipeline = nullptr;
			VkPipelineLayout PipelineLayout = nullptr;
		} NativeData;
	};

	class VulkanRHIBindingSetLayout : public RHIData {
	public:
		VulkanRHIBindingSetLayout() {}
		virtual ~VulkanRHIBindingSetLayout() {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkDescriptorSetLayout Layout = nullptr;
		} NativeData;
	};

	class VulkanRHIBindingSet : public RHIData {
	public:
		VulkanRHIBindingSet() {}
		virtual ~VulkanRHIBindingSet() override {}

		virtual void* GetNativeData() override { return (void*)&NativeData; }

	public:

		struct Data {

			VkDescriptorSet Set = nullptr;
			VkDescriptorPool AllocatedPool = nullptr;
		} NativeData;
	};
}