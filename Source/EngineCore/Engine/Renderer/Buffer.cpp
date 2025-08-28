#include <Engine/Renderer/Buffer.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/FrameRenderer.h>

namespace Spike {

	void RHIBuffer::InitRHI() {

		m_RHIData = GRHIDevice->CreateBufferRHI(m_Desc);

		if (m_Desc.MemUsage == EBufferMemUsage::ECPUOnly || m_Desc.MemUsage == EBufferMemUsage::ECPUToGPU)
			m_MappedData = GRHIDevice->MapBufferMem(this);

		if (EnumHasAllFlags(m_Desc.UsageFlags, EBufferUsageFlags::EAddressable)) {
			m_GPUAddress = GRHIDevice->GetBufferGPUAddress(this);
		}
	}

	void RHIBuffer::ReleaseRHIImmediate() {

		GRHIDevice->DestroyBufferRHI(m_RHIData);
	}

	void RHIBuffer::ReleaseRHI() {

		GFrameRenderer->PushToExecQueue([data = m_RHIData]() {
			GRHIDevice->DestroyBufferRHI(data);
			});
	}
}