#include <Engine/World/World.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Application.h>

namespace Spike {

	RHIWorldProxy::RHIWorldProxy() {

		{
			BufferDesc desc{};
			desc.Size = sizeof(LightGPUData) * MAX_LIGHTS_PER_WORLD;
			desc.UsageFlags = EBufferUsageFlags::EStorage;
			desc.MemUsage = EBufferMemUsage::ECPUToGPU;

			LightsBuffer = new RHIBuffer(desc);
		}
		{
			BufferDesc desc{};
			desc.Size = sizeof(DrawIndirectCommand) * MAX_DRAW_OBJECTS_PER_WORLD;
			desc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::EIndirect;
			desc.MemUsage = EBufferMemUsage::EGPUOnly;

			DrawCommandsBuffer = new RHIBuffer(desc);
		}
		{
			BufferDesc desc{};
			desc.Size = sizeof(uint32_t) * MAX_SHADERS_PER_WORLD;
			desc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::EIndirect | EBufferUsageFlags::ECopyDst;
			desc.MemUsage = EBufferMemUsage::EGPUOnly;

			DrawCountsBuffer = new RHIBuffer(desc);
		}
		{
			BufferDesc desc{};
			desc.Size = sizeof(ObjectGPUData) * MAX_DRAW_OBJECTS_PER_WORLD;
			desc.UsageFlags = EBufferUsageFlags::EStorage;
			desc.MemUsage = EBufferMemUsage::ECPUToGPU;

			ObjectsBuffer = new RHIBuffer(desc);
		}
		{
			BufferDesc desc{};
			desc.Size = sizeof(uint32_t) * MAX_DRAW_OBJECTS_PER_WORLD;
			desc.UsageFlags = EBufferUsageFlags::EStorage | EBufferUsageFlags::ECopyDst;
			desc.MemUsage = EBufferMemUsage::EGPUOnly;

			VisibilityBuffer = new RHIBuffer(desc);
		}
	}

	void RHIWorldProxy::InitRHI() {

		LightsBuffer->InitRHI();
		DrawCommandsBuffer->InitRHI();
		DrawCountsBuffer->InitRHI();
		ObjectsBuffer->InitRHI();
		VisibilityBuffer->InitRHI();

		ObjectsVB = DenseBuffer((ObjectGPUData*)ObjectsBuffer->GetMappedData(), (uint32_t)ObjectsBuffer->GetSize());
		LightsVB = DenseBuffer((LightGPUData*)LightsBuffer->GetMappedData(), (uint32_t)LightsBuffer->GetSize());
	}

	void RHIWorldProxy::ReleaseRHI() {

		GFrameRenderer->SubmitToFrameQueue([this]() {
			LightsBuffer->ReleaseRHIImmediate();
			delete LightsBuffer;
			DrawCommandsBuffer->ReleaseRHIImmediate();
			delete DrawCommandsBuffer;
			DrawCountsBuffer->ReleaseRHIImmediate();
			delete DrawCountsBuffer;
			ObjectsBuffer->ReleaseRHIImmediate();
			delete ObjectsBuffer;
			VisibilityBuffer->ReleaseRHIImmediate();
			delete VisibilityBuffer;
			});
	}

	World::World() {
		m_Proxy = new RHIWorldProxy();
		SafeRHIResourceInit(m_Proxy);
	}

	World::~World() {
		SafeRHIResourceRelease(m_Proxy);
	}

	void World::Tick() {

		// tick the world
		{

		}
	}
}