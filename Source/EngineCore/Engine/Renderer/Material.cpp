#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	void RHIMaterial::InitRHI() {

		m_DataIndex = GShaderManager->GetMatDataIndex();
		m_Shader = GShaderManager->GetShaderFromCache(m_ShaderDesc);
	}

	void RHIMaterial::ReleaseRHIImmediate() {

		GShaderManager->ReleaseMatDataIndex(m_DataIndex);
	}

	void RHIMaterial::ReleaseRHI() {

		GFrameRenderer->SubmitToFrameQueue([dataIndex = m_DataIndex]() {
			GShaderManager->ReleaseMatDataIndex(dataIndex);
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

		SUBMIT_RENDER_COMMAND(([=]() {

			GFrameRenderer->SubmitToFrameQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).ScalarData[resource] = value;
				GShaderManager->UpdateMatData(dataIndex);
				});
			}));
	}

	void Material::SetUint(uint8_t resource, uint32_t value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		SUBMIT_RENDER_COMMAND(([=]() {

			GFrameRenderer->SubmitToFrameQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).UintData[resource] = value;
				GShaderManager->UpdateMatData(dataIndex);
				});
			}));
	}

	void Material::SetVec2(uint8_t resource, const Vec2& value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		SUBMIT_RENDER_COMMAND(([=]() {

			GFrameRenderer->SubmitToFrameQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).Float2Data[resource] = value;
				GShaderManager->UpdateMatData(dataIndex);
				});
			}));
	}

	void Material::SetVec4(uint8_t resource, const Vec4& value) {

		uint32_t dataIndex = m_RHIResource->GetDataIndex();

		SUBMIT_RENDER_COMMAND(([=]() {

			GFrameRenderer->SubmitToFrameQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).Float4Data[resource] = value;
				GShaderManager->UpdateMatData(dataIndex);
				});
			}));
	}

	void Material::SetTextureSRV(uint8_t resource, Ref<Texture2D> value) {

		m_Textures[resource] = value;
		RHITextureView* view = value->GetResource()->GetTextureView();

		SUBMIT_RENDER_COMMAND(([=]() {

			uint32_t dataIndex = m_RHIResource->GetDataIndex();
			uint32_t texIndex = view->GetMaterialIndex();
			uint32_t samplerIndex = view->GetSourceTexture()->GetSampler()->GetMaterialIndex();

			GFrameRenderer->SubmitToFrameQueue([=]() {
				GShaderManager->GetMaterialData(dataIndex).TextureData[resource] = texIndex;
				GShaderManager->GetMaterialData(dataIndex).SamplerData[resource] = samplerIndex;
				GShaderManager->UpdateMatData(dataIndex);
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