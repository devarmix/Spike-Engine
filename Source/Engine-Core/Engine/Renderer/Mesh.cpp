#include <Engine/Renderer/Mesh.h>
#include <Platforms/Vulkan/VulkanMesh.h>
using namespace Spike;

namespace SpikeEngine {

	std::vector<Ref<Mesh>> Mesh::Create(std::filesystem::path filePath) {

		std::vector<Ref<VulkanMesh>> internalMeshes = VulkanMesh::Create(filePath);

		std::vector<Ref<Mesh>> meshes;
		meshes.reserve(internalMeshes.size());

		for (int i = 0; i < internalMeshes.size(); i++) {

			meshes.push_back(internalMeshes[i]);
		}

		return meshes;
	}
}