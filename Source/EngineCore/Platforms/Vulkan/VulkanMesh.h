#pragma once

#include <Platforms/Vulkan/VulkanBuffer.h>
#include <Engine/Renderer/Mesh.h>
using namespace SpikeEngine;

#include <filesystem>

namespace Spike {

	struct VulkanMeshData {

		VulkanBuffer IndexBuffer;
		VulkanBuffer VertexBuffer;
		VkDeviceAddress VertexBufferAddress;
	};

	class VulkanMesh : public Mesh {
	public:

		VulkanMesh() = default;
		virtual ~VulkanMesh() override { Destroy();  ASSET_CORE_DESTROY(); }

		void Destroy();

		virtual void* GetData() override { return &Data; }

		static std::vector<Ref<VulkanMesh>> Create(std::filesystem::path& filePath);

	private:

		void GenerateTangents(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices);
		void UploadGPUData(std::span<uint32_t> indices, std::span<Vertex> vertices);

	public:

		VulkanMeshData Data;
	};
}
