#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	void RHIMaterial::InitRHI() {

		// TODO - FIX ME PLEASE, 
		// the buffer is updated when its still might be in use by gpu and can cause crash 
		m_DataIndex = GShaderManager->GetMaterialDataIndex();
		m_Shader = GShaderManager->GetShaderFromCache(m_ShaderDesc);
	}

	void RHIMaterial::ReleaseRHIImmediate() {

		GShaderManager->ReleaseMaterialDataIndex(m_DataIndex);
	}

	void RHIMaterial::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([dataIndex = m_DataIndex]() {
			GShaderManager->ReleaseMaterialDataIndex(dataIndex);
			});
	}

	Material::Material(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc) {

		CreateResource(surfaceType, shaderDesc);
		m_Textures.resize(16);
	}


	Material::~Material() {

		ReleaseResource();
		m_Textures.clear();
	}

	Ref<Material> Material::Create(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc) {

		return CreateRef<Material>(surfaceType, shaderDesc);
	}

	void Material::SetScalar(uint8_t resource, float value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		EXECUTE_ON_RENDER_THREAD(([=]() {

			GFrameRenderer->PushToExecQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).ScalarData[resource] = value;
				GShaderManager->UpdateMaterialData(dataIndex);
				});
			}));
	}

	void Material::SetUint(uint8_t resource, uint32_t value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		EXECUTE_ON_RENDER_THREAD(([=]() {

			GFrameRenderer->PushToExecQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).UintData[resource] = value;
				GShaderManager->UpdateMaterialData(dataIndex);
				});
			}));
	}

	void Material::SetVec2(uint8_t resource, const Vec2& value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		EXECUTE_ON_RENDER_THREAD(([=]() {

			GFrameRenderer->PushToExecQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).Float2Data[resource] = value;
				GShaderManager->UpdateMaterialData(dataIndex);
				});
			}));
	}

	void Material::SetVec4(uint8_t resource, const Vec4& value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		EXECUTE_ON_RENDER_THREAD(([=]() {

			GFrameRenderer->PushToExecQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).Float4Data[resource] = value;
				GShaderManager->UpdateMaterialData(dataIndex);
				});
			}));
	}

	void Material::SetTextureSRV(uint8_t resource, Ref<Texture2D> value) {

		m_Textures[resource] = value;
		RHITextureView* view = value->GetResource()->GetTextureView();

		EXECUTE_ON_RENDER_THREAD(([=]() {

			uint32_t dataIndex = m_RHIResource->GetDataIndex();
			uint32_t texIndex = view->GetSRVIndex();
			uint32_t samplerIndex = view->GetSourceTexture()->GetSampler()->GetShaderIndex();

			GFrameRenderer->PushToExecQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).TextureData[resource] = texIndex;
				GShaderManager->GetMaterialData(dataIndex).SamplerData[resource] = samplerIndex;
				GShaderManager->UpdateMaterialData(dataIndex);
				});
			}));
	}

	void Material::ReleaseResource() {

		SafeRHIResourceRelease(m_RHIResource);
		m_RHIResource = nullptr;
	}

	void Material::CreateResource(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc) {

		m_RHIResource = new RHIMaterial(surfaceType, shaderDesc);
		SafeRHIResourceInit(m_RHIResource);
	}
}