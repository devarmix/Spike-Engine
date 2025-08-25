#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Renderer/FrameRenderer.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

namespace Spike {

	RHIMesh::RHIMesh(const MeshDesc& desc) : m_Desc(desc) {

		BufferDesc bufferDesc{};
		bufferDesc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopyDst;
		bufferDesc.MemUsage = EBufferMemUsage::EGPUOnly;
		bufferDesc.Size = sizeof(Vertex) * desc.Vertices.size();

		m_VertexBuffer = new RHIBuffer(bufferDesc);
		bufferDesc.Size = sizeof(uint32_t) * desc.Indices.size();
		m_IndexBuffer = new RHIBuffer(bufferDesc);
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

	Mesh::Mesh(const MeshDesc& desc) {

		CreateResource(desc);
	}

	Mesh::~Mesh() {

		ReleaseResource();
		//ASSET_CORE_DESTROY();
	}

	Ref<Mesh> Mesh::Create(const MeshDesc& desc) {

		return CreateRef<Mesh>(desc);
	}

	std::vector<Ref<Mesh>> Mesh::Create(const std::filesystem::path& filePath) {

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

		std::vector<Ref<Mesh>> meshes;

		for (fastgltf::Mesh& mesh : gltf.meshes) {

			MeshDesc meshDesc{};
			std::vector<SubMesh> subMeshes;

			for (auto&& p : mesh.primitives) {

				SubMesh newSubMesh;
				newSubMesh.FirstIndex = (uint32_t)meshDesc.Indices.size();
				newSubMesh.IndexCount = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

				size_t initial_vtx = meshDesc.Vertices.size();

				// load indicies
				{
					fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
					meshDesc.Indices.reserve(meshDesc.Indices.size() + indexAccessor.count);

					fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor, [&](std::uint32_t idx) {
						meshDesc.Indices.push_back(uint32_t(idx + initial_vtx));
						});
				}

				// load vertex positions
				{
					fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
					meshDesc.Vertices.resize(meshDesc.Vertices.size() + posAccessor.count);

					fastgltf::iterateAccessorWithIndex<Vec3>(gltf, posAccessor, [&](Vec3 v, size_t index) {

						Vertex newVtx;
						newVtx.Position = {v.x, v.y, v.z, 0};
						newVtx.Normal = { 1, 0, 0, 0 };
						newVtx.Color = Vec4(1.f);
						newVtx.Tangent = { 0, 0, 0, 0 };
						meshDesc.Vertices[initial_vtx + index] = newVtx;
						});
				}

				// load vertex normals
				auto normals = p.findAttribute("NORMAL");
				if (normals != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<Vec3>(gltf, gltf.accessors[(*normals).second], [&](Vec3 v, size_t index) {
						meshDesc.Vertices[initial_vtx + index].Normal = {v.x, v.y, v.z, 0};
						});
				}

				// load UVs
				auto uv = p.findAttribute("TEXCOORD_0");
				if (uv != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<Vec2>(gltf, gltf.accessors[(*uv).second], [&](Vec2 v, size_t index) {

						meshDesc.Vertices[initial_vtx + index].Position.w = v.x;
						meshDesc.Vertices[initial_vtx + index].Normal.w = v.y;
						});
				}

				// load vertex colors
				auto colors = p.findAttribute("COLOR_0");
				if (colors != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<Vec4>(gltf, gltf.accessors[(*colors).second], [&](Vec4 v, size_t index) {

						meshDesc.Vertices[initial_vtx + index].Color = v;
						});
				}

				// load vertex tangents
				auto tangents = p.findAttribute("TANGENT");
				if (tangents != p.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<Vec4>(gltf, gltf.accessors[(*tangents).second], [&](Vec4 v, size_t index) {

						meshDesc.Vertices[initial_vtx + index].Tangent = v;
						});
				}
				else
					assert(false);

				// calculate bounds for sub-mesh
				{
					Vec3 minpos = meshDesc.Vertices[initial_vtx].Position;
					Vec3 maxpos = meshDesc.Vertices[initial_vtx].Position;

					for (size_t i = initial_vtx; i < meshDesc.Vertices.size(); i++) {

						Vec3 pos = meshDesc.Vertices[i].Position;

						minpos = glm::min(minpos, pos);
						maxpos = glm::max(maxpos, pos);
					}

					Vec3 origin = (maxpos + minpos) / 2.f;
					Vec3 extents = (maxpos - minpos) / 2.f;

					newSubMesh.BoundsOrigin = Vec3(origin.x, origin.y, origin.z);
					newSubMesh.BoundsExtents = Vec3(extents.x, extents.y, extents.z);
					newSubMesh.BoundsRadius = glm::length(extents);
					//newSubMesh.BoundsRadius = 1.f;
				}

				subMeshes.push_back(newSubMesh);
			}

			//newMesh->GenerateTangents(indices, vertices);
			//newMesh->UploadGPUData(indices, vertices);

			Ref<Mesh> newMesh = Mesh::Create(meshDesc);
			newMesh->SubMeshes = std::move(subMeshes);

			meshes.emplace_back(newMesh);

			ENGINE_INFO("Loaded mesh with: {0} submeshes.", newMesh->SubMeshes.size());
		}

		return meshes;
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