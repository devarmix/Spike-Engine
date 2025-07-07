#include <Engine/Asset/Asset.h>

std::string Spike::GetStringFromAssetType(Spike::AssetType type) {

	switch (type)
	{
	case Spike::None:
		return "None";
		break;
	case Spike::Texture2DAsset:
		return "Texture2D";
		break;
	case Spike::CubeTextureAsset:
		return "CubeTexture";
		break;
	case Spike::MaterialAsset:
		return "Material";
		break;
	case Spike::MeshAsset:
		return "Mesh";
		break;
	default:
		return "Invalid";
		break;
	}
}

Spike::AssetType Spike::GetAssetTypeFromString(std::string s) {

	if (s == "None") return AssetType::None;
	if (s == "Texture2D") return AssetType::Texture2DAsset;
	if (s == "CubeTexture") return AssetType::CubeTextureAsset;
	if (s == "Material") return AssetType::MaterialAsset;
	if (s == "Mesh") return AssetType::MeshAsset;

	return AssetType::None;
}