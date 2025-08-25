#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Core/Log.h>

#include <fstream>
#include <regex>

Spike::ShaderManager* Spike::GShaderManager = nullptr;

namespace Spike {

	void RHIBindingSetLayout::InitRHI() {

		m_RHIData = GRHIDevice->CreateBindingSetLayoutRHI(m_Desc);
	}
	
	void RHIBindingSetLayout::ReleaseRHI() {

		GRHIDevice->DestroyBindingSetLayoutRHI(m_RHIData);
	}

	void RHIBindingSet::InitRHI() {

		m_RHIData = GRHIDevice->CreateBindingSetRHI(m_Layout);
	}

	void RHIBindingSet::ReleaseRHI() {

		GRHIDevice->DestroyBindingSetRHI(m_RHIData);
	}


    RHIShader::RHIShader(const ShaderDesc& desc, ShaderCompiler::BinaryShader& binaryShader) : m_Desc(desc), m_RHIData(nullptr) {

		if (desc.Type == EShaderType::EVertex || desc.Type == EShaderType::EGraphics) m_BinaryShader.VertexRange = std::move(binaryShader.VertexRange);
		if (desc.Type == EShaderType::EPixel || desc.Type == EShaderType::EGraphics) m_BinaryShader.PixelRange = std::move(binaryShader.PixelRange);
		if (desc.Type == EShaderType::ECompute) m_BinaryShader.ComputeRange = std::move(binaryShader.ComputeRange);

		m_BinaryShader.Hash = binaryShader.Hash;
		m_BinaryShader.MaterialData.PScalar = std::move(binaryShader.MaterialData.PScalar);
		m_BinaryShader.MaterialData.PUint = std::move(binaryShader.MaterialData.PUint);
		m_BinaryShader.MaterialData.PVec2 = std::move(binaryShader.MaterialData.PVec2);
		m_BinaryShader.MaterialData.PVec4 = std::move(binaryShader.MaterialData.PVec4);
		m_BinaryShader.MaterialData.PTexture = std::move(binaryShader.MaterialData.PTexture);
		m_BinaryShader.ShaderData = binaryShader.ShaderData;

		m_ResourcesData = (uint8_t*)malloc(m_BinaryShader.ShaderData.SizeOfResources);
	}

	RHIShader::~RHIShader() {

		free(m_ResourcesData);
	}

	void RHIShader::InitRHI() {

		m_RHIData = GRHIDevice->CreateShaderRHI(m_Desc, m_BinaryShader);

		m_BinaryShader.VertexRange.clear();
		m_BinaryShader.PixelRange.clear();
		m_BinaryShader.ComputeRange.clear();
	}

	void RHIShader::ReleaseRHI() {

		GRHIDevice->DestroyShaderRHI(m_RHIData);
	}

	void RHIShader::SetFloat(uint8_t resource, float value) {

		memcpy(m_ResourcesData + resource, (uint8_t*)&value, sizeof(float));
	}

	void RHIShader::SetUint(uint8_t resource, uint32_t value) {

		memcpy(m_ResourcesData + resource, (uint8_t*)&value, sizeof(uint32_t));
	}

	void RHIShader::SetVec2(uint8_t resource, const Vec2& value) {

		memcpy(m_ResourcesData + resource, (uint8_t*)&value, sizeof(Vec2));
	}

	void RHIShader::SetVec4(uint8_t resource, const Vec4& value) {

		memcpy(m_ResourcesData + resource, (uint8_t*)&value, sizeof(Vec4));
	}

	void RHIShader::SetMat2x2(uint8_t resource, const Mat2x2& value) {

		memcpy(m_ResourcesData + resource, (uint8_t*)&value, sizeof(Mat2x2));
	}

	void RHIShader::SetMat4x4(uint8_t resource, const Mat4x4& value) {

		memcpy(m_ResourcesData + resource, (uint8_t*)&value, sizeof(Mat4x4));
	}

	void RHIShader::SetTextureSRV(uint8_t resource, RHITextureView* value) {

		uint32_t indices[2] = {

			value->GetSRVIndex(),
			value->GetSourceTexture()->GetSampler()->GetShaderIndex()
		};

		memcpy(m_ResourcesData + resource, (uint8_t*)indices, sizeof(uint32_t) * 2);
	}

	void RHIShader::SetTextureUAVReadOnly(uint8_t resource, RHITextureView* value) {

		uint32_t indices[2] = {

			value->GetUAVReadOnlyIndex(),
			value->GetSourceTexture()->GetSampler()->GetShaderIndex()
		};

		memcpy(m_ResourcesData + resource, (uint8_t*)indices, sizeof(uint32_t) * 2);
	}

	void RHIShader::SetTextureUAV(uint8_t resource, RHITextureView* value) {

		uint32_t index = value->GetUAVIndex();
		memcpy(m_ResourcesData + resource, (uint8_t*)&index, sizeof(uint32_t));
	}

	void RHIShader::SetBufferSRV(uint8_t resource, RHIBuffer* value) {

		uint32_t index = value->GetSRVIndex();
		memcpy(m_ResourcesData + resource, (uint8_t*)&index, sizeof(uint32_t));
	}

	void RHIShader::SetBufferUAV(uint8_t resource, RHIBuffer* value) {

		uint32_t index = value->GetUAVIndex();
		memcpy(m_ResourcesData + resource, (uint8_t*)&index, sizeof(uint32_t));
	}


	ShaderManager::ShaderManager() {

		{
			BindingSetLayoutDesc desc{};
			desc.Bindings = {

				{.Type = EShaderResourceType::ETextureSRV, .Count = 1000, .Slot = 0},
				{.Type = EShaderResourceType::ETextureUAV, .Count = 100, .Slot = 1},
				{.Type = EShaderResourceType::ETextureUAV, .Count = 100, .Slot = 2},
				{.Type = EShaderResourceType::ETextureUAV, .Count = 100, .Slot = 3},
				{.Type = EShaderResourceType::ETextureSRV, .Count = 100, .Slot = 4},
				{.Type = EShaderResourceType::EBufferSRV, .Count = 100, .Slot = 5},
				{.Type = EShaderResourceType::EBufferUAV, .Count = 100, .Slot = 6},
				{.Type = EShaderResourceType::ESampler, .Count = 100, .Slot = 7},
				{.Type = EShaderResourceType::EBufferSRV, .Count = 1, .Slot = 8}
			};
			desc.UseDescriptorIndexing = true;

			m_BindlessLayout = new RHIBindingSetLayout(desc);
			m_BindlessLayout->InitRHI();

			m_BindlessSet = new RHIBindingSet(m_BindlessLayout);
			m_BindlessSet->InitRHI();
		}

		{
			BindingSetLayoutDesc desc{};
			desc.Bindings = {

				{.Type = EShaderResourceType::EConstantBuffer, .Count = 1, .Slot = 0}
			};
			desc.UseDescriptorIndexing = false;

			m_SceneLayout = new RHIBindingSetLayout(desc);
			m_SceneLayout->InitRHI();
		}

		m_MaterialDataBuffer = nullptr;
	}

	ShaderManager::~ShaderManager() {

		m_BindlessLayout->ReleaseRHIImmediate();
		delete m_BindlessLayout;

		m_BindlessSet->ReleaseRHIImmediate();
		delete m_BindlessSet;

		m_SceneLayout->ReleaseRHIImmediate();
		delete m_SceneLayout;

		m_MaterialDataBuffer->ReleaseRHIImmediate();
		delete m_MaterialDataBuffer;

		FreeShaderCache();
	}

	uint32_t ShaderManager::GetTextureSRVIndex(RHITextureView* texture, bool isReadOnlyUAV) {

		uint32_t arrayElement = 0;
		uint32_t slot = 0;

		if (texture->GetSourceTexture()->GetTextureType() == ETextureType::E2D) {

			arrayElement = m_Texture2DSRVQueue.GetFreeIndex();
		}
		else if (texture->GetSourceTexture()->GetTextureType() == ETextureType::ECube) {

			arrayElement = m_CubeTextureSRVQueue.GetFreeIndex();
			slot = 4;
		}
		else {

			ENGINE_ERROR("Non cube or 2D texture shader SRVs are not supported!");
			return INVALID_SHADER_INDEX;
		}

		m_BindlessSet->AddResourceWrite({.Slot = slot, .ArrayElement = arrayElement, .Type = EShaderResourceType::ETextureSRV, .Texture = texture, .TextureAccess = isReadOnlyUAV ? EGPUAccessFlags::EUAV : EGPUAccessFlags::ESRV});
		return arrayElement;
	}

	uint32_t ShaderManager::GetTextureUAVIndex(RHITextureView* texture) {

		if (texture->GetSourceTexture()->GetTextureType() == ETextureType::E2D) {

			uint32_t arrayElement = 0;
			uint32_t slot = 0;
			ETextureFormat format = texture->GetSourceTexture()->GetFormat();

			if (format == ETextureFormat::ERGBA16F || format == ETextureFormat::ERGBA32F) {
				slot = 3;
				arrayElement = m_Texture2DUAVFloat4Queue.GetFreeIndex();
			}
			else if (format == ETextureFormat::ERG16F) {
				slot = 2;
				arrayElement = m_Texture2DUAVFloat2Queue.GetFreeIndex();
			}
			else if (format == ETextureFormat::ER32F) {
				slot = 1;
				arrayElement = m_Texture2DUAVFloatQueue.GetFreeIndex();
			}
			else {

				ENGINE_ERROR("Non float texture shader UAVs are not supported!");
				return INVALID_SHADER_INDEX;
			}

			m_BindlessSet->AddResourceWrite({ .Slot = slot, .ArrayElement = arrayElement, .Type = EShaderResourceType::ETextureUAV, .Texture = texture, .TextureAccess = EGPUAccessFlags::EUAV });
			return arrayElement;
		}

		ENGINE_ERROR("None 2D texture shader UAVs are not supported!");
		return INVALID_SHADER_INDEX;
	}

	uint32_t ShaderManager::GetBufferSRVIndex(RHIBuffer* buffer) {

		uint32_t arrayElement = m_BufferSRVQueue.GetFreeIndex();
		m_BindlessSet->AddResourceWrite({ .Slot = 5, .ArrayElement = arrayElement, .Type = EShaderResourceType::EBufferSRV, .Buffer = buffer, .BufferRange = buffer->GetSize(), .BufferOffset = 0 });

		return arrayElement;
	}

	uint32_t ShaderManager::GetBufferUAVIndex(RHIBuffer* buffer) {

		uint32_t arrayElement = m_BufferUAVQueue.GetFreeIndex();
		m_BindlessSet->AddResourceWrite({ .Slot = 6, .ArrayElement = arrayElement, .Type = EShaderResourceType::EBufferUAV, .Buffer = buffer, .BufferRange = buffer->GetSize(), .BufferOffset = 0 });

		return arrayElement;
	}

	uint32_t ShaderManager::GetSamplerIndex(RHISampler* sampler) {

		uint32_t arrayElement = m_SamplerQueue.GetFreeIndex();
		m_BindlessSet->AddResourceWrite({ .Slot = 7, .ArrayElement = arrayElement, .Type = EShaderResourceType::ESampler, .Sampler = sampler});

		return arrayElement;
	}

	void ShaderManager::ReleaseTextureSRVIndex(uint32_t index, ETextureType textureType) {

		if (textureType == ETextureType::E2D)
			m_Texture2DSRVQueue.ReleaseIndex(index);
		else if (textureType == ETextureType::ECube)
			m_CubeTextureSRVQueue.ReleaseIndex(index);
		else
			ENGINE_ERROR("Non cube or 2D texture shader SRVs are not supported!");
	}

	void ShaderManager::ReleaseTextureUAVIndex(uint32_t index, ETextureType textureType, ETextureFormat format) {

		if (textureType == ETextureType::E2D) {

			if (format == ETextureFormat::ERGBA32F || format == ETextureFormat::ERGBA16F)
				m_Texture2DUAVFloat4Queue.ReleaseIndex(index);
			else if (format == ETextureFormat::ERG16F)
				m_Texture2DUAVFloat2Queue.ReleaseIndex(index);
			else if (format == ETextureFormat::ER32F)
				m_Texture2DUAVFloatQueue.ReleaseIndex(index);
			else 
				ENGINE_ERROR("Non float texture shader UAVs are not supported!");
		}
		else 
			ENGINE_ERROR("None 2D texture shader UAVs are not supported!")
	}

	void ShaderManager::ReleaseBufferSRVIndex(uint32_t index) {

		m_BufferSRVQueue.ReleaseIndex(index);
	}

	void ShaderManager::ReleaseBufferUAVIndex(uint32_t index) {

		m_BufferUAVQueue.ReleaseIndex(index);
	}

	void ShaderManager::ReleaseSamplerIndex(uint32_t index) {

		m_SamplerQueue.ReleaseIndex(index);
	}

	uint32_t ShaderManager::GetMaterialDataIndex() {

		uint32_t index = m_MaterialDataQueue.GetFreeIndex();
		MaterialData& data = GetMaterialData(index);

		for (int i = 0; i < 16; i++) {

			data.TextureData[i] = INVALID_SHADER_INDEX;
			data.SamplerData[i] = INVALID_SHADER_INDEX;
		}

		UpdateMaterialData(index);
		return index;
	}

	void ShaderManager::ReleaseMaterialDataIndex(uint32_t index) {

		m_MaterialDataQueue.ReleaseIndex(index);
	}

	void ShaderManager::UpdateMaterialData(uint32_t index) {

		m_BindlessSet->AddResourceWrite({.Slot = 8, .ArrayElement = 0, .Type = EShaderResourceType::EBufferSRV, .Buffer = m_MaterialDataBuffer, .BufferRange = sizeof(MaterialData), .BufferOffset = sizeof(MaterialData) * index});
	}

	void ShaderManager::InitMaterialDataBuffer() {

		BufferDesc desc{ .Size = sizeof(MaterialData) * 500, .UsageFlags = EBufferUsageFlags::EStorage, .MemUsage = EBufferMemUsage::ECPUToGPU };

		m_MaterialDataBuffer = new RHIBuffer(desc);
		m_MaterialDataBuffer->InitRHI();
	}

	ShaderManager::MaterialData& ShaderManager::GetMaterialData(uint32_t index) {

		MaterialData* data = (MaterialData*)m_MaterialDataBuffer->GetMappedData();
		return data[index];
	}

	uint32_t ShaderManager::FIFOIndexQueue::GetFreeIndex() {

		uint32_t index = INVALID_SHADER_INDEX;

		if (Queue.size() > 0) {

			index = Queue.front();
			Queue.pop_front();
		}
		else {

			index = NextIndex;
			NextIndex++;
		}

		return index;
	}

	void ShaderManager::FIFOIndexQueue::ReleaseIndex(uint32_t index) {

		Queue.push_back(index);
	}


	void ShaderManager::FreeShaderCache() {

		for (auto& [k, v] : m_ShaderCache) {

			v->ReleaseRHIImmediate();
			delete v;
		}

		m_ShaderCache.clear();
	}

	RHIShader* ShaderManager::GetShaderFromCache(const ShaderDesc& desc) {

		auto it = m_ShaderCache.find(desc);
		if (it != m_ShaderCache.end()) {
			return it->second;
		}
		else {

			std::string cachePath = "C:/Users/Artem/Desktop/Spike-Engine/Resources/Hlsl-Shaders/Cache/";
			std::string binaryPath = cachePath + desc.Name + ".bin";
			std::ifstream shaderBinary(binaryPath, std::ios::binary);

			if (shaderBinary.is_open()) {

				if (!ShaderCompiler::ValidateBinary(shaderBinary)) {

					ENGINE_ERROR("Corrupted shader binary: {0}, shader name: {1}! Undefined behavior", binaryPath, desc.Name);
					return nullptr;
				}

				ShaderCompiler::BinaryShader binShader{};
				binShader.Deserialize(shaderBinary);

				RHIShader* newShader = new RHIShader(desc, binShader);
				newShader->InitRHI();
				m_ShaderCache[desc] = newShader;

				return newShader;
			}
			else {

				ENGINE_ERROR("Failed to open cached shader from path: {0}, shader name: {1}! Undefined behavior", binaryPath, desc.Name);
				return nullptr;
			}
		}
	}
}