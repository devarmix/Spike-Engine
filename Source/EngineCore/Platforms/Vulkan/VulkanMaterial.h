#pragma once

#include <Platforms/Vulkan/VulkanCommon.h>
#include <Engine/Renderer/Material.h>
using namespace SpikeEngine;

#include <Platforms/Vulkan/VulkanBuffer.h>

#include <Platforms/Vulkan/VulkanTexture.h>
#include <Platforms/Vulkan/VulkanShader.h>

namespace Spike {

	struct VulkanMaterialManager {

		VkDescriptorPool DataPool;
		VkDescriptorSet DataSet;
		VkDescriptorSetLayout DataSetLayout;

		VulkanBuffer DataBuffer;

		static constexpr uint32_t IndexInvalid = 10000000;

		std::deque<uint32_t> FreeTextureIndicies;
		std::deque<uint32_t> FreeBufferIndicies;

		uint32_t NextTextureIndex;
		uint32_t NextBufferIndex;

		uint32_t GetFreeTextureIndex();
		uint32_t GetFreeBufferIndex();

		void ReleaseTextureIndex(uint32_t index);
		void ReleaseBufferIndex(uint32_t index);
		
		void WriteTexture(uint32_t index, VulkanTextureData* texData);
		void WriteBuffer(uint32_t index);

		void Init(uint32_t maxMatSets);
		void Cleanup();

		struct MaterialPushConstants {

			uint32_t BufIndex;
			uint32_t extra[3];

			glm::mat4 WorldMatrix;
			VkDeviceAddress VertexBuffer;
		};
		

		struct MaterialDataConstants {

			glm::vec4 ColorData[16];
			float ScalarData[48];

			uint32_t TexIndex[16];
		};
	};

	struct VulkanMaterialData {

		uint32_t BufIndex;

		struct {

			VkPipeline Pipeline;
			VkPipelineLayout Layout;

		} Pipeline;
	};

	class VulkanMaterial : public Material {
	public:

		VulkanMaterial(const VulkanMaterialData& data) : m_Data(data) {}
		virtual ~VulkanMaterial() override { Destroy(); ASSET_CORE_DESTROY(); }

		void Destroy();

		static Ref<VulkanMaterial> Create();

		virtual const void* GetData() const override { return (void*) & m_Data; }
		const VulkanMaterialData* GetRawData() const { return &m_Data; }

		virtual void SetScalarParameter(const std::string& name, float value) override;
		virtual void SetColorParameter(const std::string& name, Vector4 value) override;
		virtual void SetTextureParameter(const std::string& name, Ref<Texture> value) override;

		virtual const float GetScalarParameter(const std::string& name) override;
		virtual const Vector4 GetColorParameter(const std::string& name) override;
		virtual const Ref<Texture> GetTextureParameter(const std::string& name) override;

	public:

		virtual void AddScalarParameter(const std::string& name, uint8_t valIndex, float value) override;
		virtual void AddColorParameter(const std::string& name, uint8_t valIndex, glm::vec4 value) override;
		virtual void AddTextureParameter(const std::string& name, uint8_t valIndex, Ref<Texture> value) override;

		virtual void RemoveScalarParameter(const std::string& name) override;
		virtual void RemoveColorParameter(const std::string& name) override;
		virtual void RemoveTextureParameter(const std::string& name) override;

	private:

		void BuildPipeline(VulkanShader shader, MaterialSurfaceType surfaceType);

	private:

		VulkanMaterialData m_Data;
	};
}
