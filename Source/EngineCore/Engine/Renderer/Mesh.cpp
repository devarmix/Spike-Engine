#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

namespace Spike {

	void MeshResource::InitGPUData() {

		m_GPUData = GGfxDevice->CreateMeshGPUData(m_Vertices, m_Indices);

		if (!m_NeedCPUData) {

			m_Vertices.clear();
			m_Indices.clear();
		}
	}

	void MeshResource::ReleaseGPUData() {

		GGfxDevice->DestroyMeshGPUData(m_GPUData);
	}
}

namespace SpikeEngine {

	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMesh>& subMeshes, bool needCPUData) : SubMeshes(subMeshes) {

		CreateResource(vertices, indices, needCPUData);
	}

	Mesh::~Mesh() {

		ReleaseResource();
		ASSET_CORE_DESTROY();
	}

	Ref<Mesh> Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<SubMesh>& subMeshes, bool needCPUData) {

		return CreateRef<Mesh>(vertices, indices, subMeshes, needCPUData);
	}

	std::vector<Ref<Mesh>> Mesh::Create(std::filesystem::path filePath) {

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

		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;
		std::vector<SubMesh> subMeshes;

		for (fastgltf::Mesh& mesh : gltf.meshes) {

			subMeshes.clear();

			indices.clear();
			vertices.clear();

			for (auto&& p : mesh.primitives) {

				SubMesh newSubMesh;
				newSubMesh.FirstIndex = (uint32_t)indices.size();
				newSubMesh.IndexCount = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

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
						newVtx.Position = {v.x, v.y, v.z, 0};
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

				// calculate bounds for sub-mesh
				{
					glm::vec3 minpos = vertices[initial_vtx].Position;
					glm::vec3 maxpos = vertices[initial_vtx].Position;

					for (size_t i = initial_vtx; i < vertices.size(); i++) {

						glm::vec3 pos = vertices[i].Position;

						minpos = glm::min(minpos, pos);
						maxpos = glm::max(maxpos, pos);
					}

					glm::vec3 origin = (maxpos + minpos) / 2.f;
					glm::vec3 extents = (maxpos - minpos) / 2.f;

					newSubMesh.BoundsOrigin = Vector3(origin.x, origin.y, origin.z);
					newSubMesh.BoundsExtents = Vector3(extents.x, extents.y, extents.z);
					newSubMesh.BoundsRadius = glm::length(extents);
					//newSubMesh.BoundsRadius = 1.f;
				}

				subMeshes.push_back(newSubMesh);
			}

			//newMesh->GenerateTangents(indices, vertices);
			//newMesh->UploadGPUData(indices, vertices);

			meshes.emplace_back(Mesh::Create(vertices, indices, subMeshes));

			ENGINE_INFO("Loaded mesh with: {0} submeshes.", subMeshes.size());
		}

		return meshes;
	}

	void Mesh::ReleaseResource() {

		SafeRenderResourceRelease(m_RenderResource);
		m_RenderResource = nullptr;
	}

	void Mesh::CreateResource(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, bool needCPUData) {

		m_RenderResource = new MeshResource(vertices, indices, needCPUData);
		SafeRenderResourceInit(m_RenderResource);
	}
}