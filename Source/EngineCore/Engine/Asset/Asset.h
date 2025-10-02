#pragma once

#include <Engine/Asset/UUID.h>
#include <Engine/Core/Core.h>
#include <Engine/Utils/Misc.h>
#include <Engine/Serialization/FileStream.h>

#define ASSET_CLASS_TYPE(type)                                                 \
static Spike::EAssetType GetStaticType() { return type; }                      \
virtual Spike::EAssetType GetType() const override { return GetStaticType(); }

namespace Spike {

	enum class EAssetType : uint8_t {

		ENone = 0,

		ETexture2D,
		ECubeTexture,
		EMaterial,
		EMesh
	};

	class Asset : public RefCounted {
	public:
		virtual ~Asset() = default;
		virtual EAssetType GetType() const = 0;

		UUID GetUUID() const { return m_ID; }
		virtual void AddRef() const override final;
		virtual void Release() const override final;

	protected:
		UUID m_ID;
	};

	class AssetRegistry {
	public:
		virtual ~AssetRegistry() = default;

		virtual Ref<Asset> LoadAsset(UUID id) = 0;
		virtual void UnloadAsset(UUID id) = 0;
		virtual void Save() = 0;
		virtual void Deserialize() = 0;
	};

	// registry of current loaded project / game
	extern AssetRegistry* GRegistry;
}