#pragma once

#include <Engine/Asset/Asset.h>
#include <Engine/Events/Event.h>
#include <Engine/Renderer/CubeTexture.h>

namespace Spike {

	class EditorRegistry : public AssetRegistry {
	public:
		EditorRegistry() {}
		virtual ~EditorRegistry() override {}

		struct AssetInfo {
			EAssetType Type;
			std::filesystem::path Path;

			friend BinaryWriteStream& operator<<(BinaryWriteStream& stream, const AssetInfo& self) {
				stream << self.Type << self.Path.string();
				return stream;
			}
			friend BinaryReadStream& operator>>(BinaryReadStream& stream, AssetInfo& self) {
				std::string str{};
				stream >> self.Type >> str;

				self.Path = std::filesystem::path(std::move(str));
				return stream;
			}
		};

		virtual Ref<Asset> LoadAsset(UUID id) override;
		virtual void UnloadAsset(UUID id) override;
		virtual void Save() override;
		virtual void Deserialize() override;

		void ImportAsset(const AssetInfo& info);
		void RemoveAsset(UUID id);

	private:
		std::unordered_map<UUID, AssetInfo, UUID::Hasher> m_Registry;
		std::unordered_map<UUID, Ref<Asset>, UUID::Hasher> m_LoadedAssets;
	};

	class AssetImportedEvent : public GenericEvent {
	public:
		AssetImportedEvent(const EditorRegistry::AssetInfo& info) : m_Info(info) {}
		~AssetImportedEvent() {}

		const EditorRegistry::AssetInfo& GetInfo() const { return m_Info; }

		virtual std::string GetName() const override { return "Asset Imported"; }
		EVENT_CLASS_TYPE(EAssetImported);

	private:
		EditorRegistry::AssetInfo m_Info;
	};

	struct Texture2DImportDesc {

		ETextureFormat Format;
		bool MipMap = true;

		ESamplerFilter Filter;
		ESamplerAddress AddressU;
		ESamplerAddress AddressV;
	};

	struct CubeTextureImportDesc {

		uint32_t Size;
		ETextureFormat Format;
		ECubeTextureFilterMode FilterMode;

		ESamplerFilter Filter;
		ESamplerAddress AddressU;
		ESamplerAddress AddressV;
		ESamplerAddress AddressW;
	};

	namespace AssetImporter {

		void ImportMesh(const std::filesystem::path& sourcePath, const std::filesystem::path& assetPath);
		void ImportTexture2D(const std::filesystem::path& sourcePath, const std::filesystem::path& assetPath, Texture2DImportDesc desc);
		void ImportCubeTexture(const std::filesystem::path& sourcePath, const std::filesystem::path& assetPath, CubeTextureImportDesc desc);

		// for filtered maps (radiance, irradiance)
		void ImportCubeTexture(UUID sourceID, const std::filesystem::path& assetPath, CubeTextureImportDesc desc);
	}
}