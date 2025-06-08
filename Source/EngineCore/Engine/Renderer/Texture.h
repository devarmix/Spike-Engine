#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Math/Vector3.h>
#include <Engine/Asset/Asset.h>

namespace SpikeEngine {

	class Texture : public Spike::Asset {
	public:
		virtual ~Texture() = default;

		static Ref<Texture> Create(const char* filePath);

		virtual const Vector3 GetSize() const = 0;
		virtual const void* GetData() const = 0;

		ASSET_CLASS_TYPE(TextureAsset);
	};
}