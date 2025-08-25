#include <Engine/Renderer/Buffer.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	void RHIBuffer::InitRHI() {

		m_RHIData = GRHIDevice->CreateBufferRHI(m_Desc);

		if (m_Desc.MemUsage == EBufferMemUsage::ECPUOnly || m_Desc.MemUsage == EBufferMemUsage::ECPUToGPU)
			m_MappedData = GRHIDevice->MapBufferMem(this);

		if (m_Desc.MemUsage != EBufferMemUsage::ECPUOnly) {

			if (EnumHasAnyFlags(m_Desc.UsageFlags, EBufferUsageFlags::EStorage)) {

				m_SRVIndex = GShaderManager->GetBufferSRVIndex(this);
				m_UAVIndex = GShaderManager->GetBufferUAVIndex(this);
			}
		}
	}

	void RHIBuffer::ReleaseRHIImmediate() {

		GRHIDevice->DestroyBufferRHI(m_RHIData);

		if (m_SRVIndex != INVALID_SHADER_INDEX)
			GShaderManager->ReleaseBufferSRVIndex(m_SRVIndex);
		if (m_UAVIndex != INVALID_SHADER_INDEX)
			GShaderManager->ReleaseBufferUAVIndex(m_UAVIndex);
	}

	void RHIBuffer::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([data = m_RHIData, srv = m_SRVIndex, uav = m_UAVIndex]() {

			GRHIDevice->DestroyBufferRHI(data);

			if (srv != INVALID_SHADER_INDEX)
				GShaderManager->ReleaseBufferSRVIndex(srv);
			if (uav != INVALID_SHADER_INDEX)
				GShaderManager->ReleaseBufferUAVIndex(uav);
			});
	}
}