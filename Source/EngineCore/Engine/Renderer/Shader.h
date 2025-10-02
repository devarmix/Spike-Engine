#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Utils/Misc.h>
#include <Engine/Renderer/Texture2D.h>
#include <Engine/Renderer/CubeTexture.h>
#include <Engine/Renderer/Buffer.h>

#include <CompilerInclude.h>

#define INVALID_SHADER_INDEX UINT32_MAX

namespace Spike {

	enum class EShaderType : uint8_t {

		ENone = 0,
		EVertex,
		EPixel,

		// both vertex and fragment shaders in one file
		EGraphics,
		ECompute
	};

	enum class EShaderResourceType : uint8_t {

		ENone = 0,
		ETextureSRV,
		ETextureUAV,
		EBufferSRV,
		EBufferUAV,
		EConstantBuffer,
		ESampler
	};

	struct BindingSetLayoutDesc {

		struct Binding {

			EShaderResourceType Type;
			uint32_t Count;
			uint32_t Slot;
		};

		std::vector<Binding> Bindings;
		bool UseDescriptorIndexing = false;
	};

	class RHIBindingSetLayout : public RHIResource {
	public:
		RHIBindingSetLayout(const BindingSetLayoutDesc& desc) : m_Desc(desc), m_RHIData(0) {}
		virtual ~RHIBindingSetLayout() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		RHIData GetRHIData() const { return m_RHIData; }
		const BindingSetLayoutDesc& GetDesc() const { return m_Desc; }

	private:

		RHIData m_RHIData;
		BindingSetLayoutDesc m_Desc;
	};

	struct BindingSetWriteDesc {

		uint32_t Slot;
		uint32_t ArrayElement;
		EShaderResourceType Type;

		RHITextureView* Texture = nullptr;
		EGPUAccessFlags TextureAccess;

		RHIBuffer* Buffer = nullptr;
		size_t BufferRange;
		size_t BufferOffset;

		RHISampler* Sampler = nullptr;
	};

	class RHIBindingSet : public RHIResource {
	public:
		RHIBindingSet(RHIBindingSetLayout* layout) : m_Layout(layout), m_RHIData(0) {}
		virtual ~RHIBindingSet() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		void AddTextureWrite(uint32_t slot, uint32_t arrayEl, EShaderResourceType type, RHITextureView* view, EGPUAccessFlags access);
		void AddBufferWrite(uint32_t slot, uint32_t arrayEl, EShaderResourceType type, RHIBuffer* buffer, size_t range, size_t offset);
		void AddSamplerWrite(uint32_t slot, uint32_t arrayEl, EShaderResourceType type, RHISampler* sampler);

		void ClearWrites() { m_Writes.clear(); }
		const std::vector<BindingSetWriteDesc>& GetWrites() const { return m_Writes; }

		RHIBindingSetLayout* GetLayout() { return m_Layout; }
		RHIData GetRHIData() const { return m_RHIData; }

	private:

		std::vector<BindingSetWriteDesc> m_Writes;
		RHIBindingSetLayout* m_Layout;
		RHIData m_RHIData;
	};

	enum class EFrontFace : uint8_t {

		ENone = 0,
		EClockWise,
		ECounterClockWise
	};

	struct ShaderDesc {

		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		bool CullBackFaces = false;
		bool EnableAlphaBlend = false;
		bool EnableAdditiveBlend = false;

		EFrontFace FrontFace = EFrontFace::EClockWise;

		EShaderType Type;
		std::string Name;

		// settings for vertex / fragment shaders
		std::vector<ETextureFormat> ColorTargetFormats;

		bool operator==(const ShaderDesc& other) const {

			if (!(Type == other.Type
				&& Name.size() == other.Name.size()
				&& ColorTargetFormats.size() == other.ColorTargetFormats.size()
				&& EnableDepthTest == other.EnableDepthTest
				&& EnableDepthWrite == other.EnableDepthWrite
				&& CullBackFaces == other.CullBackFaces
				&& FrontFace == other.FrontFace))
				return false;

			for (uint32_t i = 0; i < ColorTargetFormats.size(); i++) {

				if (ColorTargetFormats[i] != other.ColorTargetFormats[i]) return false;
			}

			if (Name != other.Name) return false;
			return true;
		}

		struct Hasher {

			size_t operator()(const ShaderDesc& desc) const {

				size_t h = std::hash<uint8_t>{}((uint8_t)desc.Type);
				MathUtils::HashCombine(h, std::hash<std::string>{}(desc.Name));

				for (auto f : desc.ColorTargetFormats) {
					MathUtils::HashCombine(h, std::hash<uint8_t>{}((uint8_t)f));
				}

				MathUtils::HashCombine(h, std::hash<bool>{}(desc.EnableDepthTest));
				MathUtils::HashCombine(h, std::hash<bool>{}(desc.EnableDepthWrite));
				MathUtils::HashCombine(h, std::hash<bool>{}(desc.CullBackFaces));
				MathUtils::HashCombine(h, std::hash<uint8_t>{}((uint8_t)desc.FrontFace));
				return h;
			}
		};
	};

	class RHIShader : public RHIResource {
	public:
		RHIShader(const ShaderDesc& desc, ShaderCompiler::BinaryShader& binaryShader);
		virtual ~RHIShader() override;

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		EShaderType GetShaderType() const { return m_Desc.Type; }
		const std::string& GetName() const { return m_Desc.Name; }
		const ShaderCompiler::BinaryShader::MaterialMetadata& GetMaterialData() const { return m_BinaryShader.MaterialData; }

		RHIData GetRHIData() const { return m_RHIData; }
		const ShaderDesc& GetDesc() const { return m_Desc; }
		const std::vector<RHIBindingSetLayout*>& GetLayouts() const { return m_Layouts; }
		uint32_t GetPushDataSize() const { return m_BinaryShader.ShaderData.PushDataSize; }

	private:

		RHIData m_RHIData;
		ShaderDesc m_Desc;

		ShaderCompiler::BinaryShader m_BinaryShader;
		std::vector<RHIBindingSetLayout*> m_Layouts;
	};

	class ShaderManager {
	public:
		ShaderManager();
		~ShaderManager();

		void FreeShaderCache();
		RHIShader* GetShaderFromCache(const ShaderDesc& desc);

		uint32_t GetMatTextureIndex(RHITextureView* texture);
		uint32_t GetMatSamplerIndex(RHISampler* sampler);
		void ReleaseMatTextureIndex(uint32_t index);
		void ReleaseMatSamplerIndex(uint32_t index);

		uint32_t GetMatDataIndex();
		void ReleaseMatDataIndex(uint32_t index);
		void UpdateMatData(uint32_t index);
		void InitMatDataBuffer();

		struct alignas(16) MaterialData {

			float ScalarData[16];
			uint32_t UintData[16];
			Vec2 Float2Data[16];
			Vec4 Float4Data[16];
			uint32_t TextureData[16];
			uint32_t SamplerData[16];
		};

		MaterialData& GetMaterialData(uint32_t index);
		RHIBindingSet* GetMaterialSet() { return m_MaterialSet; }
		RHIBindingSetLayout* GetMeshDrawLayout() { return m_MeshDrawLayout; }
		RHIBindingSetLayout* GetMaterialLayout() { return m_MaterialLayout; }

	private:

		IndexQueue m_MatTextureQueue;
		IndexQueue m_MatSamplerQueue;
		IndexQueue m_MatDataQueue;
		RHIBuffer* m_MaterialDataBuffer;

		RHIBindingSetLayout* m_MaterialLayout;
		RHIBindingSetLayout* m_MeshDrawLayout;
		RHIBindingSet* m_MaterialSet;

		std::unordered_map<ShaderDesc, RHIShader*, ShaderDesc::Hasher> m_ShaderCache;
	};

	// global shader data manager pointer
	extern ShaderManager* GShaderManager;
}