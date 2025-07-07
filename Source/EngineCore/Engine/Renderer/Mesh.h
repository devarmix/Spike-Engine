#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Asset/Asset.h>

#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/RenderResource.h>
#include <Engine/Math/Vector3.h>
#include <filesystem>
#include <glm/glm.hpp>
using namespace Spike;

namespace Spike {

	struct Vertex {

		glm::vec4 Position;           // w - UV_x
		glm::vec4 Normal;             // w - UV_y
		glm::vec4 Color;
		glm::vec4 Tangent;            // w - handedness
	};

	class MeshResource : public RenderResource {
	public:

		MeshResource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, bool needCPUData) 
			: m_Vertices(vertices), m_Indices(indices), m_NeedCPUData(needCPUData), m_GPUData(nullptr) {}

		virtual ~MeshResource() override { m_Vertices.clear(); m_Indices.clear(); }

		ResourceGPUData* GetGPUData() { return m_GPUData; }

		virtual void InitGPUData() override;
		virtual void ReleaseGPUData() override;

	private:

		ResourceGPUData* m_GPUData;

		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indices;

		bool m_NeedCPUData;
	};
}

namespace SpikeEngine {

	struct SubMesh {

		uint32_t FirstIndex;
		uint32_t IndexCount;

		Vector3 BoundsOrigin;
		Vector3 BoundsExtents;
		float BoundsRadius;

		Ref<Material> Material;
	};

	class Mesh : public Asset {
	public:

		Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMesh>& subMeshes, bool needCPUData);
		virtual ~Mesh() override;

		static Ref<Mesh> Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMesh>& subMeshes, bool needCPUData = false);
		static std::vector<Ref<Mesh>> Create(std::filesystem::path filePath);

		MeshResource* GetResource() { return m_RenderResource; }
		void ReleaseResource();
		void CreateResource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, bool needCPUData = false);

		ASSET_CLASS_TYPE(MeshAsset)

	public:

		std::vector<SubMesh> SubMeshes;

	private:

		MeshResource* m_RenderResource;
	};
}