#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Math/Vector3.h>
#include <Engine/Asset/Asset.h>

namespace SpikeEngine {

	class CubeTexture : public Spike::Asset {
	public:

		static Ref<CubeTexture> Create(const char** filePath);

		virtual Vector3 GetSize() const = 0;
		virtual void* GetData() = 0;

		ASSET_CLASS_TYPE(CubeTextureAsset);
	};
}