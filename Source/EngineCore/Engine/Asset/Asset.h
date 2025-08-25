#pragma once

#include <Engine/Asset/GUID.h>
#include <string>

namespace Spike {

	enum AssetType : uint8_t {

		EAssetNone = 0,

		EAssetTexture2D,
		EAssetCubeTexture,
		EAssetMaterial,
		EAssetMaterialInstance,
		EAssetMesh
	};

#define ASSET_CORE_DESTROY()

#define ASSET_CLASS_TYPE(type)                                                          \
        static Spike::AssetType GetStaticType() { return Spike::AssetType::type; }      \
        virtual Spike::AssetType GetType() const override { return GetStaticType(); }

	class Asset {
	public:

		virtual ~Asset() = default;
		//virtual AssetType GetType() const = 0;

	public:

		AssetID Guid;
	};
}