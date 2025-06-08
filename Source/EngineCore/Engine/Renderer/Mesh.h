#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Asset/Asset.h>

#include <Engine/Renderer/Material.h>

#include <filesystem>

namespace SpikeEngine {

	struct MeshBounds {

		Vector3 Origin;
		Vector3 Extents;

		float Radius;
	};

	struct SubMesh {

		uint32_t StartVertexIndex;
		uint32_t VertexCount;

		MeshBounds Bounds;
		Ref<Material> Material;
	};

	class Mesh : public Spike::Asset {
	public:
		virtual ~Mesh() = default;

		static std::vector<Ref<Mesh>> Create(std::filesystem::path filePath);

		virtual const void* GetData() const = 0;

		ASSET_CLASS_TYPE(MeshAsset);

	public:

		std::vector<SubMesh> SubMeshes;
	};
}