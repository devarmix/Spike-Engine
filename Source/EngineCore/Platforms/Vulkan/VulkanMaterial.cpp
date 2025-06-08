#include <Platforms/Vulkan/VulkanMaterial.h>
#include <Platforms/Vulkan/VulkanDescriptors.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

#include <Platforms/Vulkan/VulkanPipeline.h>
#include <Platforms/Vulkan/VulkanTools.h>

namespace Spike {

	void VulkanMaterial::SetScalarParameter(const std::string& name, float value) {

		auto it = m_ScalarParameters.find(name);
		if (it != m_ScalarParameters.end()) {

			uint32_t bufIndex = m_Data.BufIndex;
			uint8_t valIndex = m_ScalarParameters[name].DataBindIndex;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			constants[bufIndex].ScalarData[valIndex] = value;

			VulkanRenderer::GlobalMaterialManager.WriteBuffer(bufIndex);
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name: {0} does not exist!", name);
		}
	}

	void VulkanMaterial::SetColorParameter(const std::string& name, Vector4 value) {

		auto it = m_ColorParameters.find(name);
		if (it != m_ColorParameters.end()) {

			uint32_t bufIndex = m_Data.BufIndex;
			uint8_t valIndex = m_ColorParameters[name].DataBindIndex;
			glm::vec4 val = glm::vec4{ value.x, value.y, value.z, value.w };

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			constants[bufIndex].ColorData[valIndex] = val;

			VulkanRenderer::GlobalMaterialManager.WriteBuffer(bufIndex);
		}
		else {

			ENGINE_ERROR("Color parameter with the name: {0} does not exist!", name);
		}
	}

	void VulkanMaterial::SetTextureParameter(const std::string& name, Ref<Texture> value) {

		auto it = m_TextureParameters.find(name);
		if (it != m_TextureParameters.end()) {

			uint8_t valIndex = m_TextureParameters[name].DataBindIndex;
			uint32_t bufIndex = m_Data.BufIndex;

			m_TextureParameters[name].Value = value;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			uint32_t texIndex = constants[bufIndex].TexIndex[valIndex];

			VulkanTextureData* texData = (VulkanTextureData*)value->GetData();
			VulkanRenderer::GlobalMaterialManager.WriteTexture(texIndex, texData);
		}
		else {

			ENGINE_ERROR("Texture parameter with the name: {0} does not exist!", name);
		}
	}

	const float VulkanMaterial::GetScalarParameter(const std::string& name) {

		auto it = m_ScalarParameters.find(name);
		if (it != m_ScalarParameters.end()) {

			uint32_t bufIndex = m_Data.BufIndex;
			uint8_t valIndex = m_ScalarParameters[name].DataBindIndex;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			return constants[bufIndex].ScalarData[valIndex];
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name : {0} does not exist!", name);
			return 0;
		}
	}

	const Vector4 VulkanMaterial::GetColorParameter(const std::string& name) {

		auto it = m_ColorParameters.find(name);
		if (it != m_ColorParameters.end()) {

			uint32_t bufIndex = m_Data.BufIndex;
			uint8_t valIndex = m_ColorParameters[name].DataBindIndex;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			glm::vec4 val = constants[bufIndex].ColorData[valIndex];

			return Vector4(val.x, val.y, val.z, val.w);
		}
		else {

			ENGINE_ERROR("Color parameter with the name : {0} does not exist!", name);
			return Vector4::Zero();
		}
	}

	const Ref<Texture> VulkanMaterial::GetTextureParameter(const std::string& name) {

		auto it = m_TextureParameters.find(name);
		if (it != m_TextureParameters.end()) {

			return m_TextureParameters[name].Value;
		}
		else {

			ENGINE_ERROR("Texture parameter with the name : {0} does not exist!", name);
			return nullptr;
		}
	}

	void VulkanMaterial::AddScalarParameter(const std::string& name, uint8_t valIndex, float value) {

		auto it = m_ScalarParameters.find(name);
		if (it == m_ScalarParameters.end()) {

			ScalarParameter newParam{ valIndex };
			m_ScalarParameters[name] = newParam;

			SetScalarParameter(name, value);
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name: {0} already exists!", name);
		}
	}

	void VulkanMaterial::AddColorParameter(const std::string& name, uint8_t valIndex, glm::vec4 value) {

		auto it = m_ColorParameters.find(name);
		if (it == m_ColorParameters.end()) {

			ColorParameter newParam{ valIndex };
			m_ColorParameters[name] = newParam;

			SetColorParameter(name, Vector4(value.x, value.y, value.z, value.w));
		}
		else {

			ENGINE_ERROR("Color parameter with the name: {0} already exists!", name);
		}
 	}

	void VulkanMaterial::AddTextureParameter(const std::string& name, uint8_t valIndex, Ref<Texture> value) {

		auto it = m_TextureParameters.find(name);
		if (it == m_TextureParameters.end()) {

			uint32_t newTexIndex = VulkanRenderer::GlobalMaterialManager.GetFreeTextureIndex();
			uint32_t bufIndex = m_Data.BufIndex;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			constants[bufIndex].TexIndex[valIndex] = newTexIndex;

			VulkanRenderer::GlobalMaterialManager.WriteBuffer(bufIndex);

			TextureParameter newParam{ value, valIndex };
			m_TextureParameters[name] = newParam;

			SetTextureParameter(name, value);
		}
		else {

			ENGINE_ERROR("Texture parameter with the name: {0} already exists!", name);
		}
	}

	void VulkanMaterial::RemoveScalarParameter(const std::string& name) {

		auto it = m_ScalarParameters.find(name);
		if (it != m_ScalarParameters.end()) {

			m_ScalarParameters.erase(name);
		}
		else {

			ENGINE_ERROR("Scalar parameter with the name : {0} does not exist!", name);
		}
	}

	void VulkanMaterial::RemoveColorParameter(const std::string& name) {

		auto it = m_ColorParameters.find(name);
		if (it != m_ColorParameters.end()) {

			m_ColorParameters.erase(name);
		}
		else {

			ENGINE_ERROR("Color parameter with the name : {0} does not exist!", name);
		}
	}

	void VulkanMaterial::RemoveTextureParameter(const std::string& name) {

		auto it = m_TextureParameters.find(name);
		if (it != m_TextureParameters.end()) {

			uint8_t valIndex = m_TextureParameters[name].DataBindIndex;
			uint32_t bufIndex = m_Data.BufIndex;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			VulkanRenderer::GlobalMaterialManager.ReleaseTextureIndex(constants[bufIndex].TexIndex[valIndex]);

			constants[bufIndex].TexIndex[valIndex] = VulkanMaterialManager::IndexInvalid;

			m_TextureParameters.erase(name);
		}
		else {

			ENGINE_ERROR("Texture parameter with the name : {0} does not exist!", name);
		}
	}

	void VulkanMaterial::BuildPipeline(VulkanShader* shader, MaterialSurfaceType surfaceType) {

		VkPushConstantRange matrixRange{};
		matrixRange.offset = 0;
		matrixRange.size = sizeof(VulkanMaterialManager::MaterialPushConstants);
		matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayout layouts[] = { VulkanRenderer::SceneDataDescriptorLayout, VulkanRenderer::GlobalMaterialManager.DataSetLayout };

		VkPipelineLayoutCreateInfo mesh_Layout_Info = VulkanTools::PipelineLayoutCreateInfo();
		mesh_Layout_Info.setLayoutCount = 2;
		mesh_Layout_Info.pSetLayouts = layouts;
		mesh_Layout_Info.pPushConstantRanges = &matrixRange;
		mesh_Layout_Info.pushConstantRangeCount = 1;

		VkPipelineLayout newLayout;
		VK_CHECK(vkCreatePipelineLayout(VulkanRenderer::Device.Device, &mesh_Layout_Info, nullptr, &newLayout));

		m_Data.Pipeline.Layout = newLayout;

		PipelineBuilder pipelineBulder;
		pipelineBulder.SetShaders(shader->VertexModule, shader->FragmentModule);
		pipelineBulder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipelineBulder.SetPolygonMode(VK_POLYGON_MODE_FILL);
		pipelineBulder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		pipelineBulder.SetMultisamplingNone();

		VkFormat colorAttachmentFormats[3];
		colorAttachmentFormats[0] = VulkanRenderer::GBuffer.AlbedoMap->GetRawData()->Format;
		colorAttachmentFormats[1] = VulkanRenderer::GBuffer.NormalMap->GetRawData()->Format;
		//colorAttachmentFormats[2] = VulkanRenderer::GBuffer.PositionMap->Data.Format;
		colorAttachmentFormats[2] = VulkanRenderer::GBuffer.MaterialMap->GetRawData()->Format;

		pipelineBulder.SetColorAttachments(colorAttachmentFormats, 3);
		pipelineBulder.SetDepthFormat(VulkanRenderer::GBuffer.DepthMap->GetRawData()->Format);

		pipelineBulder.PipelineLayout = newLayout;

		if (surfaceType == Opaque) {

			pipelineBulder.DisableBlending();
			pipelineBulder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_Data.Pipeline.Pipeline = pipelineBulder.BuildPipeline(VulkanRenderer::Device.Device);
		} 
		else if (surfaceType == Transparent) {

			pipelineBulder.EnableBlendingAdditive();

			pipelineBulder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

			m_Data.Pipeline.Pipeline = pipelineBulder.BuildPipeline(VulkanRenderer::Device.Device);
		}
	}

	Ref<VulkanMaterial> VulkanMaterial::Create() {

		VulkanMaterialData matData{};

		uint32_t bufIndex = VulkanRenderer::GlobalMaterialManager.GetFreeBufferIndex();

		// create new material buffer
		{
			VulkanMaterialManager::MaterialDataConstants newBuf{};

			for (int i = 0; i < 16; i++) {

				newBuf.TexIndex[i] = VulkanMaterialManager::IndexInvalid;
			}

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			constants[bufIndex] = newBuf;

			VulkanRenderer::GlobalMaterialManager.WriteBuffer(bufIndex);
		}

		matData.BufIndex = bufIndex;

		VulkanShader* shader = VulkanShader::Create("C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/PBRTest.vert.spv", "C:/Users/Artem/Desktop/Spike-Engine/Resources/Shaders/PBRTest.frag.spv");

		Ref<VulkanMaterial> mat = CreateRef<VulkanMaterial>(matData);

		mat->BuildPipeline(shader, Opaque);

		delete shader;

		return mat;
	}

	void VulkanMaterial::Destroy() {

		for (auto& [k, v] : m_TextureParameters) {

			uint8_t valIndex = v.DataBindIndex;
			uint32_t bufIndex = m_Data.BufIndex;

			VulkanMaterialManager::MaterialDataConstants* constants = (VulkanMaterialManager::MaterialDataConstants*)VulkanRenderer::GlobalMaterialManager.DataBuffer->AllocationInfo.pMappedData;
			VulkanRenderer::GlobalMaterialManager.ReleaseTextureIndex(constants[bufIndex].TexIndex[valIndex]);
		}

		m_TextureParameters.clear();

		VulkanRenderer::GlobalMaterialManager.ReleaseBufferIndex(m_Data.BufIndex);

		if (m_Data.Pipeline.Layout != VK_NULL_HANDLE) {

			vkDestroyPipelineLayout(VulkanRenderer::Device.Device, m_Data.Pipeline.Layout, nullptr);
			m_Data.Pipeline.Layout = VK_NULL_HANDLE;
		}

		if (m_Data.Pipeline.Pipeline != VK_NULL_HANDLE) {

			vkDestroyPipeline(VulkanRenderer::Device.Device, m_Data.Pipeline.Pipeline, nullptr);
			m_Data.Pipeline.Pipeline = VK_NULL_HANDLE;
		}
	}


	void VulkanMaterialManager::Init(uint32_t maxMatSets) {

		// Create Descriptor Set Layout
		{
			std::array<VkDescriptorSetLayoutBinding, 2> LayoutBindings{

				VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = maxMatSets * 16, .stageFlags = VK_SHADER_STAGE_ALL, .pImmutableSamplers = nullptr},

				VkDescriptorSetLayoutBinding{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = maxMatSets, .stageFlags = VK_SHADER_STAGE_ALL, .pImmutableSamplers = nullptr}
			};

			std::array<VkDescriptorBindingFlags, 2> LayoutBindingFlags{

				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
			};

			VkDescriptorSetLayoutBindingFlagsCreateInfo LayoutBindingFlagsInfo{};
			LayoutBindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			LayoutBindingFlagsInfo.pNext = nullptr;
			LayoutBindingFlagsInfo.pBindingFlags = LayoutBindingFlags.data();
			LayoutBindingFlagsInfo.bindingCount = 2;

			VkDescriptorSetLayoutCreateInfo LayoutInfo{};
			LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutInfo.bindingCount = 2;
			LayoutInfo.pBindings = LayoutBindings.data();

			LayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			LayoutInfo.pNext = &LayoutBindingFlagsInfo;

			VK_CHECK(vkCreateDescriptorSetLayout(VulkanRenderer::Device.Device, &LayoutInfo, nullptr, &DataSetLayout));
		}

		// Create Descriptor Pool
		{
			std::array<VkDescriptorPoolSize, 2> PoolSizes {

				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = maxMatSets * 16},
				VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = maxMatSets}
			};

			VkDescriptorPoolCreateInfo PoolInfo{};
			PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			PoolInfo.poolSizeCount = 2;
			PoolInfo.pPoolSizes = PoolSizes.data();
			PoolInfo.maxSets = 1;
			PoolInfo.pNext = nullptr;

			VK_CHECK(vkCreateDescriptorPool(VulkanRenderer::Device.Device, &PoolInfo, nullptr, &DataPool));
		}

		// Create Descriptor Set
		{
			VkDescriptorSetAllocateInfo SetInfo{};
			SetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			SetInfo.pNext = nullptr;
			SetInfo.descriptorPool = DataPool;
			SetInfo.pSetLayouts = &DataSetLayout;
			SetInfo.descriptorSetCount = 1;

			VK_CHECK(vkAllocateDescriptorSets(VulkanRenderer::Device.Device, &SetInfo, &DataSet));
		}

		// Allocate Data Buffer
		DataBuffer = VulkanBuffer::Create(sizeof(MaterialDataConstants) * maxMatSets, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Initialize Other Parameters
		NextBufferIndex = 0;
		NextTextureIndex = 0;

		FreeBufferIndicies.clear();
		FreeTextureIndicies.clear();
	}

	void VulkanMaterialManager::Cleanup() {

		vkDestroyDescriptorPool(VulkanRenderer::Device.Device, DataPool, nullptr);

		vkDestroyDescriptorSetLayout(VulkanRenderer::Device.Device, DataSetLayout, nullptr);

		delete DataBuffer;
	}

	uint32_t VulkanMaterialManager::GetFreeTextureIndex() {

		if (FreeTextureIndicies.size() > 0) {

			uint32_t index = FreeTextureIndicies.front();
			FreeTextureIndicies.pop_front();

			return index;
		}

		uint32_t index = NextTextureIndex;
		NextTextureIndex++;

		return index;
	}

	uint32_t VulkanMaterialManager::GetFreeBufferIndex() {

		if (FreeBufferIndicies.size() > 0) {

			uint32_t index = FreeBufferIndicies.front();
			FreeBufferIndicies.pop_front();

			return index;
		}

		uint32_t index = NextBufferIndex;
		NextBufferIndex++;

		return index;
	}

	void VulkanMaterialManager::ReleaseTextureIndex(uint32_t index) {

		// TODO: Check. if the index already is in deque, to prevent data overwriting

		FreeTextureIndicies.push_back(index);
	}

	void VulkanMaterialManager::ReleaseBufferIndex(uint32_t index) {

		// TODO: Check, if the index already is in deque, to prevent data overwriting

		FreeBufferIndicies.push_back(index);
	}

	void VulkanMaterialManager::WriteBuffer(uint32_t index) {

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = DataBuffer->Buffer;
		bufferInfo.offset = index * sizeof(MaterialDataConstants);
		bufferInfo.range = sizeof(MaterialDataConstants);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = DataSet;
		write.dstBinding = 1;
		write.dstArrayElement = index;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(VulkanRenderer::Device.Device, 1, &write, 0, nullptr);
	}

	void VulkanMaterialManager::WriteTexture(uint32_t index, VulkanTextureData* texData) {

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texData->View;
		imageInfo.sampler = VulkanRenderer::DefSamplerLinear;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = DataSet;
		write.dstBinding = 0;
		write.dstArrayElement = index;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(VulkanRenderer::Device.Device, 1, &write, 0, nullptr);
	}
}