#pragma once

#include <Platforms/Vulkan/VulkanBuffer.h>
#include <Engine/Renderer/Mesh.h>
using namespace SpikeEngine;

#include <filesystem>

namespace Spike {

	struct VulkanMeshData {

		VulkanBuffer* IndexBuffer;
		VulkanBuffer* VertexBuffer;
		VkDeviceAddress VertexBufferAddress;
	};

	class VulkanMesh : public Mesh {
	public:

		VulkanMesh(const VulkanMeshData& data) : m_Data(data) {}
		virtual ~VulkanMesh() override { Destroy();  ASSET_CORE_DESTROY(); }

		void Destroy();

		virtual const void* GetData() const override { return (void*)&m_Data; }
		const VulkanMeshData* GetRawData() const { return &m_Data; }

		static std::vector<Ref<VulkanMesh>> Create(std::filesystem::path& filePath);

	private:

		void GenerateTangents(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices);
		void UploadGPUData(std::span<uint32_t> indices, std::span<Vertex> vertices);

	private:

		VulkanMeshData m_Data;
	};
}
