#pragma once

#include <Engine/Asset/GUID.h>
#include <string>

namespace Spike {

	enum AssetType : uint8_t {

		None = 0,

		TextureAsset,
		CubeTextureAsset,
		MaterialAsset,
		MeshAsset
	};

	std::string GetStringFromAssetType(AssetType type);
	AssetType GetAssetTypeFromString(std::string s);

#define ASSET_CORE_DESTROY()

#define ASSET_CLASS_TYPE(type)                                                          \
        static Spike::AssetType GetStaticType() { return Spike::AssetType::type; }      \
        virtual Spike::AssetType GetType() const override { return GetStaticType(); }

	class Asset {
	public:

		virtual ~Asset() = default;
		virtual AssetType GetType() const = 0;

	public:

		GUID Guid;
	};
}