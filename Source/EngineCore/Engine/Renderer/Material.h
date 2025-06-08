#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Math/Vector4.h>
#include <Engine/Renderer/Texture.h>
#include <Engine/Asset/Asset.h>

#include <glm/glm.hpp>
#include <map>

namespace SpikeEngine {

	struct ScalarParameter {

		uint8_t DataBindIndex;
	};

	struct ColorParameter {

		uint8_t DataBindIndex;
	};

	struct TextureParameter {

		Ref<Texture> Value;

		uint8_t DataBindIndex;
	};

	enum MaterialSurfaceType : uint8_t {

		Opaque,
		Transparent
	};


	class Material : public Spike::Asset {
	public:
		virtual ~Material() = default;

		static Ref<Material> Create();

		virtual const void* GetData() const = 0;

		virtual void SetScalarParameter(const std::string& name, float value) = 0;
		virtual void SetColorParameter(const std::string& name, Vector4 value) = 0;
		virtual void SetTextureParameter(const std::string& name, Ref<Texture> value) = 0;

		virtual const float GetScalarParameter(const std::string& name) = 0;
		virtual const Vector4 GetColorParameter(const std::string& name) = 0;
		virtual const Ref<Texture> GetTextureParameter(const std::string& name) = 0;

		const MaterialSurfaceType GetSurfaceType() const { return m_SurfaceType; }

		ASSET_CLASS_TYPE(MaterialAsset);

	public:

		virtual void AddScalarParameter(const std::string& name, uint8_t valIndex, float value) = 0;
		virtual void AddColorParameter(const std::string& name, uint8_t valIndex, glm::vec4 value) = 0;
		virtual void AddTextureParameter(const std::string& name, uint8_t valIndex, Ref<Texture> value) = 0;

		virtual void RemoveScalarParameter(const std::string& name) = 0;
		virtual void RemoveColorParameter(const std::string& name) = 0;
		virtual void RemoveTextureParameter(const std::string& name) = 0;

		std::map<std::string, ScalarParameter> m_ScalarParameters;
		std::map<std::string, ColorParameter> m_ColorParameters;
		std::map<std::string, TextureParameter> m_TextureParameters;

		MaterialSurfaceType m_SurfaceType;
	};
}