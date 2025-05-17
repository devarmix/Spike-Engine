#include <Platforms/Vulkan/VulkanMesh.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

namespace Spike {

	void VulkanMesh::GenerateTangents(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices) {

		
	}

	std::vector<Ref<VulkanMesh>> VulkanMesh::Create(std::filesystem::path& filePath) {

		ENGINE_WARN("Loading GLTF: " + filePath.string());

		fastgltf::GltfDataBuffer data;
		data.loadFromFile(filePath);

		constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

		fastgltf::Asset gltf;
		fastgltf::Parser parser{};

		auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		}
		else {

			ENGINE_ERROR("Failed To Load GLTF: {}", fastgltf::to_underlying(load.error()));
			return {};
		}

		std::vector<Ref<VulkanMesh>> meshes;

		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;

		for (fastgltf::Mesh& mesh : gltf.meshes) {

			VulkanMeshData meshData{};
			Ref<VulkanMesh> newMesh = CreateRef<VulkanMesh>(meshData);

			indices.clear();
			vertices.clear();

			for (auto&& p : mesh.primitives) {

				SubMesh newSubMesh;
				newSubMesh.StartVertexIndex = (uint32_t)indices.size();
				newSubMesh.VertexCount = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

				size_t initial_vtx = vertices.size();

				// load indicies
				{
					fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
					indices.reserve(indices.size() + indexAccessor.count);

					fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor, [&](std::uint32_t idx) {
						indices.push_back(uint32_t(idx + initial_vtx));
				    });
				}

				// load vertex positions
				{
					fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
					vertices.resize(vertices.size() + posAccessor.count);

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {

						Vertex newVtx;
						newVtx.Position = { v.x, v.y, v.z, 0 };
						newVtx.Normal = { 1, 0, 0, 0 };
						newVtx.Color = glm::vec4{ 1.f };
						newVtx.Tangent = { 0, 0, 0, 0 };
						vertices[initial_vtx + index] = newVtx;
					});
				}

				// load vertex normals
				auto normals = p.findAttribute("NORMAL");
				if (normals != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second], [&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].Normal = {v.x, v.y, v.z, 0};
					});
				}

				// load UVs
				auto uv = p.findAttribute("TEXCOORD_0");
				if (uv != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second], [&](glm::vec2 v, size_t index) {

						vertices[initial_vtx + index].Position.w = v.x;
						vertices[initial_vtx + index].Normal.w = v.y;
					});
				}

				// load vertex colors
				auto colors = p.findAttribute("COLOR_0");
				if (colors != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second], [&](glm::vec4 v, size_t index) {

						vertices[initial_vtx + index].Color = v;
					});
				}

				// load vertex tangents
				auto tangents = p.findAttribute("TANGENT");
				if (tangents != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*tangents).second], [&](glm::vec4 v, size_t index) {

						vertices[initial_vtx + index].Tangent = v;
					});
				}
				else
					assert(false);

				newMesh->SubMeshes.push_back(newSubMesh);
			}

			constexpr bool OverrideColors = false;
			if (OverrideColors) {

				for (Vertex& vtx : vertices) {

					vtx.Color = glm::vec4(vtx.Normal.x, vtx.Normal.y, vtx.Normal.z, 1.f);
				}
			}

			//newMesh->GenerateTangents(indices, vertices);
			newMesh->UploadGPUData(indices, vertices);

			meshes.emplace_back(newMesh);
		}

		return meshes;
	}

	void VulkanMesh::UploadGPUData(std::span<uint32_t> indices, std::span<Vertex> vertices) {

		const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		m_Data.VertexBuffer = VulkanBuffer::Create(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		VkBufferDeviceAddressInfo deviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = m_Data.VertexBuffer.Buffer };
		m_Data.VertexBufferAddress = vkGetBufferDeviceAddress(VulkanRenderer::Device.Device, &deviceAddressInfo);

		m_Data.IndexBuffer = VulkanBuffer::Create(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		VulkanBuffer staging = VulkanBuffer::Create(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(VulkanRenderer::Device.Allocator, staging.Allocation, &data);
		
		// copy vertex and index buffers
		memcpy(data, vertices.data(), vertexBufferSize);
		memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

		vmaUnmapMemory(VulkanRenderer::Device.Allocator, staging.Allocation);

		VulkanRenderer::ImmediateSubmit([&](VkCommandBuffer cmd) {

			VkBufferCopy vertexCopy{ 0 };
			vertexCopy.dstOffset = 0;
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vertexBufferSize;

			vkCmdCopyBuffer(cmd, staging.Buffer, m_Data.VertexBuffer.Buffer, 1, &vertexCopy);

			VkBufferCopy indexCopy{ 0 };
			indexCopy.dstOffset = 0;
			indexCopy.srcOffset = vertexBufferSize;
			indexCopy.size = indexBufferSize;

			vkCmdCopyBuffer(cmd, staging.Buffer, m_Data.IndexBuffer.Buffer, 1, &indexCopy);
		});

		staging.Destroy();
	}

	void VulkanMesh::Destroy() {

		m_Data.IndexBuffer.Destroy();
		m_Data.VertexBuffer.Destroy();
	}
}