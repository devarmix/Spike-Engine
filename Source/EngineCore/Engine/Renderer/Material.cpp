#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/Application.h>

namespace Spike {

	void MaterialResource::InitGPUData() {

		m_GPUData = GGfxDevice->CreateMaterialGPUData(m_SurfaceType);
	}

	void MaterialResource::ReleaseGPUData() {

		GGfxDevice->DestroyMaterialGPUData(m_GPUData);
	}
}

namespace SpikeEngine {

	Material::Material(EMaterialSurfaceType surfaceType) {

		CreateResource(surfaceType);
	}

	Material::~Material() {

		ReleaseResource();
		ASSET_CORE_DESTROY();
	}

	Ref<Material> Material::Create(EMaterialSurfaceType surfaceType) {

		return CreateRef<Material>(surfaceType);
	}

	void Material::SetScalarParameter(const std::string& name, float value) {

		auto it = m_ScalarParameters.find(name);
		if (it != m_ScalarParameters.end()) {

			m_ScalarParameters[name].Value = value;
			uint8_t valIndex = m_ScalarParameters[name].DataBindIndex;

			EXECUTE_ON_RENDER_THREAD([=]() {
				
				GGfxDevice->SetMaterialScalarParameter(m_RenderResource, valIndex, value);
				});
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name: {0} does not exist!", name);
		}
	}

	void Material::SetColorParameter(const std::string& name, Vector4 value) {

		auto it = m_ColorParameters.find(name);
		if (it != m_ColorParameters.end()) {

			m_ColorParameters[name].Value = value;
			uint8_t valIndex = m_ColorParameters[name].DataBindIndex;
			
			EXECUTE_ON_RENDER_THREAD([=]() {
				
				GGfxDevice->SetMaterialColorParameter(m_RenderResource, valIndex, value);
				});
		}
		else {

			ENGINE_ERROR("Color parameter with the name: {0} does not exist!", name);
		}
	}

	void Material::SetTextureParameter(const std::string& name, Ref<Texture2D> value) {

		auto it = m_TextureParameters.find(name);
		if (it != m_TextureParameters.end()) {

			m_TextureParameters[name].Value = value;
			uint8_t valIndex = m_TextureParameters[name].DataBindIndex;

			Texture2DResource* texture = value->GetResource();

			EXECUTE_ON_RENDER_THREAD([=]() {

				GGfxDevice->SetMaterialTextureParameter(m_RenderResource, valIndex, texture);
				});
		}
		else {

			ENGINE_ERROR("Texture parameter with the name: {0} does not exist!", name);
		}
	}

	float Material::GetScalarParameter(const std::string& name) {

		auto it = m_ScalarParameters.find(name);
		if (it != m_ScalarParameters.end()) {

			return m_ScalarParameters[name].Value;
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name : {0} does not exist!", name);
			return 0;
		}
	}

	Vector4 Material::GetColorParameter(const std::string& name) {

		auto it = m_ColorParameters.find(name);
		if (it != m_ColorParameters.end()) {

			return m_ColorParameters[name].Value;
		}
		else {

			ENGINE_ERROR("Color parameter with the name : {0} does not exist!", name);
			return Vector4::Zero();
		}
	}

	Ref<Texture2D> Material::GetTextureParameter(const std::string& name) {

		auto it = m_TextureParameters.find(name);
		if (it != m_TextureParameters.end()) {

			return m_TextureParameters[name].Value;
		}
		else {

			ENGINE_ERROR("Texture parameter with the name : {0} does not exist!", name);
			return nullptr;
		}
	}

	void Material::ReleaseResource() {

		SafeRenderResourceRelease(m_RenderResource);
		m_RenderResource = nullptr;
	}

	void Material::CreateResource(EMaterialSurfaceType surfaceType) {

		m_RenderResource = new MaterialResource(surfaceType);
		SafeRenderResourceInit(m_RenderResource);
	}

	void Material::AddScalarParameter(const std::string& name, uint8_t valIndex, float value) {

		auto it = m_ScalarParameters.find(name);
		if (it == m_ScalarParameters.end()) {

			ScalarParameter newParam{ .Value = value, .DataBindIndex = valIndex };
			m_ScalarParameters[name] = newParam;

			EXECUTE_ON_RENDER_THREAD(([=, this]() {

				GGfxDevice->SetMaterialScalarParameter(m_RenderResource, valIndex, value);
				}));
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name: {0} already exists!", name);
		}
	}

	void Material::AddColorParameter(const std::string& name, uint8_t valIndex, Vector4 value) {

		auto it = m_ColorParameters.find(name);
		if (it == m_ColorParameters.end()) {

			ColorParameter newParam{ .Value = value, .DataBindIndex = valIndex };
			m_ColorParameters[name] = newParam;

			EXECUTE_ON_RENDER_THREAD(([=, this]() {

				GGfxDevice->SetMaterialColorParameter(m_RenderResource, valIndex, value);
				}));
		}
		else {

			ENGINE_ERROR("Color parameter with the name: {0} already exists!", name);
		}
	}

	void Material::AddTextureParameter(const std::string& name, uint8_t valIndex, Ref<Texture2D> value) {

		auto it = m_TextureParameters.find(name);
		if (it == m_TextureParameters.end()) {

			TextureParameter newParam{ .Value = value, .DataBindIndex = valIndex };
			m_TextureParameters[name] = newParam;

			Texture2DResource* texture = value->GetResource();

			EXECUTE_ON_RENDER_THREAD(([=, this]() {

				GGfxDevice->AddMaterialTextureParameter(m_RenderResource, valIndex);
				GGfxDevice->SetMaterialTextureParameter(m_RenderResource, valIndex, texture);
				}));
		}
		else {

			ENGINE_ERROR("Texture parameter with the name: {0} already exists!", name);
		}
	}

	void Material::RemoveScalarParameter(const std::string& name) {

		auto it = m_ScalarParameters.find(name);
		if (it != m_ScalarParameters.end()) {

			m_ScalarParameters.erase(name);
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name : {0} does not exist!", name);
		}
	}

	void Material::RemoveColorParameter(const std::string& name) {

		auto it = m_ColorParameters.find(name);
		if (it != m_ColorParameters.end()) {

			m_ColorParameters.erase(name);
		}
		else {

			ENGINE_ERROR("Color parameter with the name : {0} does not exist!", name);
		}
	}

	void Material::RemoveTextureParameter(const std::string& name) {

		auto it = m_TextureParameters.find(name);
		if (it != m_TextureParameters.end()) {

			uint8_t valIndex = m_TextureParameters[name].DataBindIndex;

			EXECUTE_ON_RENDER_THREAD(([=, this]() {

				GGfxDevice->RemoveMaterialTextureParameter(m_RenderResource, valIndex);
				}));

			m_TextureParameters.erase(name);
		}
		else {

			ENGINE_ERROR("Texture parameter with the name : {0} does not exist!", name);
		}
	}
}