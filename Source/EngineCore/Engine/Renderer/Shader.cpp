#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/FrameRenderer.h>
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

	void RHIBindingSet::AddTextureWrite(uint32_t slot, uint32_t arrayEl, EShaderResourceType type, RHITextureView* view, EGPUAccessFlags access) {

		m_Writes.push_back({ .Slot = slot, .ArrayElement = arrayEl, .Type = type, .Texture = view, .TextureAccess = access });
	}

	void RHIBindingSet::AddBufferWrite(uint32_t slot, uint32_t arrayEl, EShaderResourceType type, RHIBuffer* buffer, size_t range, size_t offset) {

		m_Writes.push_back({ .Slot = slot, .ArrayElement = arrayEl, .Type = type, .Buffer = buffer, .BufferRange = range, .BufferOffset = offset });
	}

	void RHIBindingSet::AddSamplerWrite(uint32_t slot, uint32_t arrayEl, EShaderResourceType type, RHISampler* sampler) {

		m_Writes.push_back({ .Slot = slot, .ArrayElement = arrayEl, .Type = type, .Sampler = sampler });
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

		if (!binaryShader.ShaderData.IsMaterialShader) {

			uint32_t setCount = 0;
			for (auto& b : binaryShader.ShaderData.Bindings) {
				setCount = std::max(setCount, b.Set + 1);
			}

			std::vector<BindingSetLayoutDesc> descs;
			descs.resize(setCount);

			for (auto& b : binaryShader.ShaderData.Bindings) {

				descs[b.Set].Bindings.push_back({ .Type = (EShaderResourceType)b.Type, .Count = b.Count, .Slot = b.Binding });
				if (b.Count > 1) {
					descs[b.Set].UseDescriptorIndexing = true;
				}
			}

			for (auto& desc : descs) {
				RHIBindingSetLayout* layout = new RHIBindingSetLayout(desc);
				m_Layouts.push_back(layout);
			}
		}
		else {
			m_Layouts = { GShaderManager->GetMeshDrawLayout(), GShaderManager->GetMaterialLayout() };
		}

		m_BinaryShader.ShaderData.PushDataSize = binaryShader.ShaderData.PushDataSize;
		m_BinaryShader.ShaderData.IsMaterialShader = binaryShader.ShaderData.IsMaterialShader;
	}

	RHIShader::~RHIShader() {}

	void RHIShader::InitRHI() {

		if (!m_BinaryShader.ShaderData.IsMaterialShader) {

			for (auto layout : m_Layouts) {
				layout->InitRHI();
			}
		}

		m_RHIData = GRHIDevice->CreateShaderRHI(m_Desc, m_BinaryShader, m_Layouts);

		m_BinaryShader.VertexRange.clear();
		m_BinaryShader.PixelRange.clear();
		m_BinaryShader.ComputeRange.clear();
	}

	void RHIShader::ReleaseRHI() {

		GRHIDevice->DestroyShaderRHI(m_RHIData);

		if (!m_BinaryShader.ShaderData.IsMaterialShader) {

			for (auto layout : m_Layouts) {
				layout->ReleaseRHI();
				delete layout;
			}
		}
	}

	ShaderManager::ShaderManager() {

		{
			BindingSetLayoutDesc desc{};
			desc.Bindings = {
				{.Type = EShaderResourceType::EConstantBuffer, .Count = 1, .Slot = 0},
				{.Type = EShaderResourceType::EBufferSRV, .Count = 100, .Slot = 1}
			};

			m_MeshDrawLayout = new RHIBindingSetLayout(desc);
			m_MeshDrawLayout->InitRHI();
		}
		{
			BindingSetLayoutDesc desc{};
			desc.Bindings = {
				{.Type = EShaderResourceType::EBufferSRV, .Count = 1, .Slot = 0},
				{.Type = EShaderResourceType::ETextureSRV, .Count = 100, .Slot = 1},
				{.Type = EShaderResourceType::ESampler, .Count = 100, .Slot = 2}
			};
			desc.UseDescriptorIndexing = true;

			m_MaterialLayout = new RHIBindingSetLayout(desc);
			m_MaterialLayout->InitRHI();

			m_MaterialSet = new RHIBindingSet(m_MaterialLayout);
			m_MaterialSet->InitRHI();
		}

		m_MaterialDataBuffer = nullptr;
	}

	ShaderManager::~ShaderManager() {

		m_MeshDrawLayout->ReleaseRHI();
		delete m_MeshDrawLayout;

		m_MaterialLayout->ReleaseRHIImmediate();
		delete m_MaterialLayout;

		m_MaterialSet->ReleaseRHIImmediate();
		delete m_MaterialSet;

		m_MaterialDataBuffer->ReleaseRHIImmediate();
		delete m_MaterialDataBuffer;

		FreeShaderCache();
	}

	uint32_t ShaderManager::GetMatTextureIndex(RHITextureView* texture) {

		uint32_t texIndex = m_MatTextureQueue.GetFreeIndex();

		m_MaterialSet->AddTextureWrite(1, texIndex, EShaderResourceType::ETextureSRV, texture, EGPUAccessFlags::ESRVGraphics);
		return texIndex;
	}

	uint32_t ShaderManager::GetMatSamplerIndex(RHISampler* sampler) {

		uint32_t samplerIndex = m_MatSamplerQueue.GetFreeIndex();

		m_MaterialSet->AddSamplerWrite(2, samplerIndex, EShaderResourceType::ESampler, sampler);
		return samplerIndex;
	}

	void ShaderManager::ReleaseMatTextureIndex(uint32_t index) {

		m_MatTextureQueue.ReleaseIndex(index);
	}

	void ShaderManager::ReleaseMatSamplerIndex(uint32_t index) {

		m_MatSamplerQueue.ReleaseIndex(index);
	}

	uint32_t ShaderManager::GetMatDataIndex() {

		uint32_t index = m_MatDataQueue.GetFreeIndex();
		MaterialData& data = GetMaterialData(index);

		GFrameRenderer->PushToExecQueue([&data]() {

			for (int i = 0; i < 16; i++) {

				data.TextureData[i] = INVALID_SHADER_INDEX;
				data.SamplerData[i] = INVALID_SHADER_INDEX;
			}
			});

		UpdateMatData(index);
		return index;
	}

	void ShaderManager::ReleaseMatDataIndex(uint32_t index) {

		m_MatDataQueue.ReleaseIndex(index);
	}

	void ShaderManager::UpdateMatData(uint32_t index) {

		//m_BindlessSet->AddResourceWrite({.Slot = 8, .ArrayElement = 0, .Type = EShaderResourceType::EBufferSRV, .Buffer = m_MaterialDataBuffer, .BufferRange = sizeof(MaterialData), .BufferOffset = sizeof(MaterialData) * index});
		m_MaterialSet->AddBufferWrite(0, 0, EShaderResourceType::EBufferSRV, m_MaterialDataBuffer, sizeof(MaterialData), sizeof(MaterialData) * index);
	}

	void ShaderManager::InitMatDataBuffer() {

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