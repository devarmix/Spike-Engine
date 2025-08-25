#pragma once

#include <Engine/Core/Core.h>
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
		RHIBindingSetLayout(const BindingSetLayoutDesc& desc) : m_Desc(desc), m_RHIData(nullptr) {}
		virtual ~RHIBindingSetLayout() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		RHIData* GetRHIData() { return m_RHIData; }
		const BindingSetLayoutDesc& GetDesc() const { return m_Desc; }

	private:

		RHIData* m_RHIData;
		BindingSetLayoutDesc m_Desc;
	};

	struct BindingSetWriteDesc {

		uint32_t Slot;
		uint32_t ArrayElement;
		EShaderResourceType Type;

		RHITextureView* Texture = nullptr;
		EGPUAccessFlags TextureAccess;

		RHIBuffer* Buffer = nullptr;
		uint64_t BufferRange;
		uint64_t BufferOffset;

		RHISampler* Sampler = nullptr;
	};

	class RHIBindingSet : public RHIResource {
	public:
		RHIBindingSet(RHIBindingSetLayout* layout) : m_Layout(layout), m_RHIData(nullptr) {}
		virtual ~RHIBindingSet() override {}

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

		void AddResourceWrite(const BindingSetWriteDesc& desc) { m_Writes.push_back(desc); }
		void ClearWrites() { m_Writes.clear(); }
		const std::vector<BindingSetWriteDesc>& GetWrites() const { return m_Writes; }

		RHIBindingSetLayout* GetLayout() { return m_Layout; }
		RHIData* GetRHIData() { return m_RHIData; }

	private:

		std::vector<BindingSetWriteDesc> m_Writes;
		RHIBindingSetLayout* m_Layout;
		RHIData* m_RHIData;
	};

	enum class EFrontFace : uint8_t {

		ENone = 0,
		EClockWise,
		ECounterClockWise
	};

	struct ShaderDesc {

		EShaderType Type;
		std::string Name;

		// settings for vertex / fragment shaders
		std::vector<ETextureFormat> ColorTargetFormats;
		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		bool CullBackFaces = false;

		EFrontFace FrontFace = EFrontFace::EClockWise;

		bool operator==(const ShaderDesc& other) const {

			if (!(Type == other.Type
				&& Name.size() == other.Name.size()
				&& ColorTargetFormats.size() == other.ColorTargetFormats.size()
				&& EnableDepthTest == other.EnableDepthTest
				&& EnableDepthWrite == other.EnableDepthWrite
				&& CullBackFaces == other.CullBackFaces))
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
				HashCombine(h, std::hash<std::string>{}(desc.Name));

				for (auto f : desc.ColorTargetFormats) {
					HashCombine(h, std::hash<uint8_t>{}((uint8_t)f));
				}

				HashCombine(h, std::hash<bool>{}(desc.EnableDepthTest));
				HashCombine(h, std::hash<bool>{}(desc.EnableDepthWrite));
				HashCombine(h, std::hash<bool>{}(desc.CullBackFaces));
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

		void SetFloat(uint8_t resource, float value);
		void SetUint(uint8_t resource, uint32_t value);
		void SetVec2(uint8_t resource, const Vec2& value);
		void SetVec4(uint8_t resource, const Vec4& value);
		void SetMat2x2(uint8_t resource, const Mat2x2& value);
		void SetMat4x4(uint8_t resource, const Mat4x4& value);
		void SetTextureSRV(uint8_t resource, RHITextureView* value);
		void SetTextureUAVReadOnly(uint8_t resource, RHITextureView* value);
		void SetTextureUAV(uint8_t resource, RHITextureView* value);
		void SetBufferSRV(uint8_t resource, RHIBuffer* value);
		void SetBufferUAV(uint8_t resource, RHIBuffer* value);

		EShaderType GetShaderType() const { return m_Desc.Type; }
		const std::string& GetName() const { return m_Desc.Name; }
		const ShaderCompiler::BinaryShader::MaterialMetadata& GetMaterialData() const { return m_BinaryShader.MaterialData; }

		const uint8_t* GetResourcesData() const { return m_ResourcesData; }
		uint32_t GetResourcesSize() const { return m_BinaryShader.ShaderData.SizeOfResources; }

		RHIData* GetRHIData() { return m_RHIData; }
		const ShaderDesc& GetDesc() const { return m_Desc; }

	private:

		RHIData* m_RHIData;
		ShaderDesc m_Desc;

		ShaderCompiler::BinaryShader m_BinaryShader;
		uint8_t* m_ResourcesData;
	};

	class ShaderManager {
	public:
		ShaderManager();
		~ShaderManager();

		void FreeShaderCache();
		RHIShader* GetShaderFromCache(const ShaderDesc& desc);

		uint32_t GetTextureSRVIndex(RHITextureView* texture, bool isReadOnlyUAV);
		uint32_t GetTextureUAVIndex(RHITextureView* texture);
		uint32_t GetBufferSRVIndex(RHIBuffer* buffer);
		uint32_t GetBufferUAVIndex(RHIBuffer* buffer);
		uint32_t GetSamplerIndex(RHISampler* sampler);

		void ReleaseTextureSRVIndex(uint32_t index, ETextureType textureType);
		void ReleaseTextureUAVIndex(uint32_t index, ETextureType textureType, ETextureFormat format);
		void ReleaseBufferSRVIndex(uint32_t index);
		void ReleaseBufferUAVIndex(uint32_t index);
		void ReleaseSamplerIndex(uint32_t index);

		uint32_t GetMaterialDataIndex();
		void ReleaseMaterialDataIndex(uint32_t index);
		void UpdateMaterialData(uint32_t index);
		void InitMaterialDataBuffer();

		struct alignas(16) MaterialData {

			float ScalarData[16];
			uint32_t UintData[16];
			Vec2 Float2Data[16];
			Vec4 Float4Data[16];
			uint32_t TextureData[16];
			uint32_t SamplerData[16];
		};

		MaterialData& GetMaterialData(uint32_t index);

		RHIBindingSet* GetBindlessSet() { return m_BindlessSet; }
		RHIBindingSetLayout* GetSceneLayout() { return m_SceneLayout; }
		RHIBindingSetLayout* GetBindlessLayout() { return m_BindlessLayout; }

	private:

		struct FIFOIndexQueue {

			FIFOIndexQueue() : NextIndex(0) { Queue.clear(); }

			uint32_t NextIndex;
			std::deque<uint32_t> Queue;

			uint32_t GetFreeIndex();
			void ReleaseIndex(uint32_t index);
		};

		FIFOIndexQueue m_Texture2DSRVQueue;
		FIFOIndexQueue m_Texture2DUAVFloatQueue;
		FIFOIndexQueue m_Texture2DUAVFloat2Queue;
		FIFOIndexQueue m_Texture2DUAVFloat4Queue;
		FIFOIndexQueue m_CubeTextureSRVQueue;
		FIFOIndexQueue m_BufferSRVQueue;
		FIFOIndexQueue m_BufferUAVQueue;
		FIFOIndexQueue m_SamplerQueue;

		RHIBindingSetLayout* m_BindlessLayout;
		RHIBindingSetLayout* m_SceneLayout;
		RHIBindingSet* m_BindlessSet;

		RHIBuffer* m_MaterialDataBuffer;
		FIFOIndexQueue m_MaterialDataQueue;

		std::unordered_map<ShaderDesc, RHIShader*, ShaderDesc::Hasher> m_ShaderCache;
	};

	// global shader data manager pointer
	extern ShaderManager* GShaderManager;
}