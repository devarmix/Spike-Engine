#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/Buffer.h>
#include <Engine/Asset/Asset.h>
#include <Engine/Renderer/Shader.h>

namespace Spike {

	enum class EMaterialSurfaceType : uint8_t {

		ENone = 0,
		EOpaque,
		ETransparent
	};

	class RHIMaterial : public RHIResource {
	public:
		RHIMaterial(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc) : m_SurfaceType(surfaceType), m_DataIndex(0), m_Shader(nullptr), m_ShaderDesc(shaderDesc) {}
		virtual ~RHIMaterial() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;
		virtual void ReleaseRHIImmediate() override;

		EMaterialSurfaceType GetSurfaceType() const { return m_SurfaceType; }
		uint32_t GetDataIndex() const { return m_DataIndex; }
		RHIShader* GetShader() { return m_Shader; }

	private:

		EMaterialSurfaceType m_SurfaceType;
		uint32_t m_DataIndex;

		RHIShader* m_Shader;
		ShaderDesc m_ShaderDesc;
	};

	class Material : public Asset {
	public:
		Material(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc);
		virtual ~Material() override;

		static Ref<Material> Create(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc);

		void SetScalar(uint8_t resource, float value);
		void SetUint(uint8_t resource, uint32_t value);
		void SetVec2(uint8_t resource, const Vec2& value);
		void SetVec4(uint8_t resource, const Vec4& value);
		void SetTextureSRV(uint8_t resource, Ref<Texture2D> value);

		EMaterialSurfaceType GetSurfaceType() const { return m_RHIResource->GetSurfaceType(); }

		RHIMaterial* GetResource() { return m_RHIResource; }
		void ReleaseResource();
		void CreateResource(EMaterialSurfaceType surfaceType, const ShaderDesc& shaderDesc);

		ASSET_CLASS_TYPE(EAssetType::ECubeTexture);

	private:

		RHIMaterial* m_RHIResource;
		std::vector<Ref<Texture2D>> m_Textures;
	};
}