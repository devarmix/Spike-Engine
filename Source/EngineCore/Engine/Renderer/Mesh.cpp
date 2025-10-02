#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	RHIMesh::RHIMesh(const MeshDesc& desc) : m_Desc(desc) {

		BufferDesc bufferDesc{};
		bufferDesc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopyDst | EBufferUsageFlags::EAddressable;
		bufferDesc.MemUsage = EBufferMemUsage::EGPUOnly;
		bufferDesc.Size = sizeof(Vertex) * desc.Vertices.size();

		m_VertexBuffer = new RHIBuffer(bufferDesc);
		bufferDesc.Size = sizeof(uint32_t) * desc.Indices.size();
		m_IndexBuffer = new RHIBuffer(bufferDesc);

		// calculate bounds
		{
			Vec3 minpos = Vec3(m_Desc.Vertices[0].Position);
			Vec3 maxpos = Vec3(m_Desc.Vertices[0].Position);

			for (int i = 0; i < m_Desc.Vertices.size(); i++) {

				Vec3 pos = Vec3(m_Desc.Vertices[i].Position);
				minpos = glm::min(minpos, pos);
				maxpos = glm::max(maxpos, pos);
			}

			Vec3 origin = (maxpos + minpos) / 2.f;
			Vec3 extents = (maxpos - minpos) / 2.f;

			m_BoundsOrigin = origin;
			m_BoundsRadius = glm::length(extents);
		}
	}

	void RHIMesh::InitRHI() {

		m_VertexBuffer->InitRHI();
		m_IndexBuffer->InitRHI();

		const size_t vertexBufferSize = sizeof(Vertex) * m_Desc.Vertices.size();
		const size_t indexBufferSize = sizeof(uint32_t) * m_Desc.Indices.size();

		BufferDesc stagingDesc{};
		stagingDesc.Size = vertexBufferSize + indexBufferSize;
		stagingDesc.UsageFlags = EBufferUsageFlags::ECopySrc;
		stagingDesc.MemUsage = EBufferMemUsage::ECPUOnly;

		RHIBuffer* staging = new RHIBuffer(stagingDesc);
		staging->InitRHI();

		memcpy(staging->GetMappedData(), m_Desc.Vertices.data(), vertexBufferSize);
		memcpy((char*)staging->GetMappedData() + vertexBufferSize, m_Desc.Indices.data(), indexBufferSize);

		GRHIDevice->ImmediateSubmit([&, this](RHICommandBuffer* cmd) {

			GRHIDevice->CopyBuffer(cmd, staging, m_VertexBuffer, 0, 0, vertexBufferSize);
			GRHIDevice->CopyBuffer(cmd, staging, m_IndexBuffer, vertexBufferSize, 0, indexBufferSize);
			});

		staging->ReleaseRHI();
		delete staging;

		if (!m_Desc.NeedCPUData) {

			m_Desc.Vertices.clear();
			m_Desc.Indices.clear();
		}
	}

	void RHIMesh::ReleaseRHI() {

		m_VertexBuffer->ReleaseRHI();
		delete m_VertexBuffer;

		m_IndexBuffer->ReleaseRHI();
		delete m_IndexBuffer;
	}

	Mesh::Mesh(const MeshDesc& desc, UUID id) {
		m_ID = id;
		CreateResource(desc);
	}

	Mesh::~Mesh() {
		ReleaseResource();
	}

	Ref<Mesh> Mesh::Create(BinaryReadStream& stream, UUID id) {
		MeshDesc desc{};

		char magic[4] = {};
		stream >> magic;

		if (memcmp(MESH_MAGIC, magic, sizeof(char) * 4) != 0) {
			ENGINE_ERROR("Mesh asset file is not valid: {}!", (uint64_t)id);
			return nullptr;
		}

		stream >> desc.Vertices >> desc.Indices >> desc.SubMeshes;
		return CreateRef<Mesh>(desc, id);
	}

	void Mesh::ReleaseResource() {

		SafeRHIResourceRelease(m_RHIResource);
		m_RHIResource = nullptr;
	}

	void Mesh::CreateResource(const MeshDesc& desc) {

		m_RHIResource = new RHIMesh(desc);
		SafeRHIResourceInit(m_RHIResource);
	}
}