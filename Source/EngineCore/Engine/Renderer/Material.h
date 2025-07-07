#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Math/Vector4.h>
#include <Engine/Renderer/Texture2D.h>
#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/RenderResource.h>
using namespace Spike;

#include <map>

namespace Spike {

	enum EMaterialSurfaceType : uint8_t {

		ESurfaceTypeOpaque,
		ESurfaceTypeTransparent
	};

	class MaterialResource : public RenderResource {
	public:

		MaterialResource(EMaterialSurfaceType surfaceType)
			: m_SurfaceType(surfaceType), m_GPUData(nullptr) {}

		virtual ~MaterialResource() override {}

		ResourceGPUData* GetGPUData() { return m_GPUData; }

		virtual void InitGPUData() override;
		virtual void ReleaseGPUData() override;

		EMaterialSurfaceType GetSurfaceType() const { return m_SurfaceType; }

	private:

		ResourceGPUData* m_GPUData;

		EMaterialSurfaceType m_SurfaceType;
	};
}

namespace SpikeEngine {

	struct ScalarParameter {

		float Value;

		uint8_t DataBindIndex;
	};

	struct ColorParameter {

		Vector4 Value;

		uint8_t DataBindIndex;
	};

	struct TextureParameter {

		Ref<Texture2D> Value;

		uint8_t DataBindIndex;
	};


	class Material : public Asset {
	public:

		Material(EMaterialSurfaceType surfaceType);
		virtual ~Material() override;

		static Ref<Material> Create(EMaterialSurfaceType surfaceType);

		void SetScalarParameter(const std::string& name, float value);
		void SetColorParameter(const std::string& name, Vector4 value);
		void SetTextureParameter(const std::string& name, Ref<Texture2D> value);

		float GetScalarParameter(const std::string& name);
		Vector4 GetColorParameter(const std::string& name);
		Ref<Texture2D> GetTextureParameter(const std::string& name);

		EMaterialSurfaceType GetSurfaceType() const { return m_RenderResource->GetSurfaceType(); }

		MaterialResource* GetResource() { return m_RenderResource; }
		void ReleaseResource();
		void CreateResource(EMaterialSurfaceType surfaceType);

		ASSET_CLASS_TYPE(MaterialAsset)

	public:

		void AddScalarParameter(const std::string& name, uint8_t valIndex, float value);
		void AddColorParameter(const std::string& name, uint8_t valIndex, Vector4 value);
		void AddTextureParameter(const std::string& name, uint8_t valIndex, Ref<Texture2D> value);

		void RemoveScalarParameter(const std::string& name);
		void RemoveColorParameter(const std::string& name);
		void RemoveTextureParameter(const std::string& name);

		std::map<std::string, ScalarParameter> m_ScalarParameters;
		std::map<std::string, ColorParameter> m_ColorParameters;
		std::map<std::string, TextureParameter> m_TextureParameters;

	private:

		MaterialResource* m_RenderResource;
	};
}