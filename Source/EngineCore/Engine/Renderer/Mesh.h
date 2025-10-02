#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Utils/MathUtils.h>
#include <Engine/Asset/Asset.h>

#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/RHIResource.h>
using namespace Spike;

namespace Spike {

	struct alignas(16) Vertex {

		Vec3 Position;
		uint32_t Color;

		Vec2 UV0;
		Vec2 UV1;

		PackedHalf Tangent;
		PackedHalf Normal;
	};

	struct SubMesh {

		uint32_t FirstIndex;
		uint32_t IndexCount;
	};

	constexpr char MESH_MAGIC[4] = { 'S', 'E', 'M', 'S' };

	struct MeshDesc {

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;
		std::vector<SubMesh> SubMeshes;

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

		Vec3 GetBoundsOrigin() const { return m_BoundsOrigin; }
		float GetBoundsRadius() const { return m_BoundsRadius; }
		const MeshDesc& GetDesc() const { return m_Desc; }

	private:

		MeshDesc m_Desc;

		RHIBuffer* m_VertexBuffer;
		RHIBuffer* m_IndexBuffer;

		Vec3 m_BoundsOrigin;
		float m_BoundsRadius;
	};

	class Mesh : public Asset {
	public:
		Mesh(const MeshDesc& desc, UUID id);
		virtual ~Mesh() override;

		static Ref<Mesh> Create(BinaryReadStream& stream, UUID id);
		
		RHIMesh* GetResource() { return m_RHIResource; }
		void ReleaseResource();
		void CreateResource(const MeshDesc& desc);

		const std::vector<Vertex>& GetVertices() const { return m_RHIResource->GetVertices(); }
		const std::vector<uint32_t>& GetIndices() const { return m_RHIResource->GetIndices(); }

		ASSET_CLASS_TYPE(EAssetType::EMesh);

	private:
		RHIMesh* m_RHIResource;
	};
}