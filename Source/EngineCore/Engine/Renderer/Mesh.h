#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Asset/Asset.h>

#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/RHIResource.h>
#include <filesystem>
#include <glm/glm.hpp>
using namespace Spike;

namespace Spike {

	struct Vertex {

		Vec4 Position;           // w - UV_x
		Vec4 Normal;             // w - UV_y
		Vec4 Color;
		Vec4 Tangent;            // w - handedness
	};

	struct SubMesh {

		uint32_t FirstIndex;
		uint32_t IndexCount;

		Vec3 BoundsOrigin;
		Vec3 BoundsExtents;
		float BoundsRadius;

		Ref<Material> Material;
	};

	struct MeshDesc {

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;
		bool NeedCPUData = false;
	};

	class RHIMesh : public RHIResource {
	public:
		RHIMesh(const MeshDesc& desc);
		virtual ~RHIMesh() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		RHIBuffer* GetVertexBuffer() { return m_VertexBuffer; }
		RHIBuffer* GetIndexBuffer() { return m_IndexBuffer; }

		const std::vector<Vertex>& GetVertices() const { return m_Desc.Vertices; }
		const std::vector<uint32_t>& GetIndices() const { return m_Desc.Indices; }
		const MeshDesc& GetDesc() const { return m_Desc; }

	private:

		MeshDesc m_Desc;

		RHIBuffer* m_VertexBuffer;
		RHIBuffer* m_IndexBuffer;
	};

	class Mesh : public Asset {
	public:
		Mesh(const MeshDesc& desc);
		virtual ~Mesh() override;

		static Ref<Mesh> Create(const MeshDesc& desc);
		static std::vector<Ref<Mesh>> Create(const std::filesystem::path& filePath);
		
		RHIMesh* GetResource() { return m_RHIResource; }
		void ReleaseResource();
		void CreateResource(const MeshDesc& desc);

		const std::vector<Vertex>& GetVertices() const { return m_RHIResource->GetVertices(); }
		const std::vector<uint32_t>& GetIndices() const { return m_RHIResource->GetIndices(); }

	public:
		std::vector<SubMesh> SubMeshes;

	private:
		RHIMesh* m_RHIResource;
	};
}