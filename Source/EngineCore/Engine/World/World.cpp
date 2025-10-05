#include <Engine/World/World.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Application.h>

Spike::World* Spike::World::s_Current = nullptr;

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

		ObjectsVB.Make((ObjectGPUData*)ObjectsBuffer->GetMappedData(), MAX_DRAW_OBJECTS_PER_WORLD);
		LightsVB.Make((LightGPUData*)LightsBuffer->GetMappedData(), MAX_LIGHTS_PER_WORLD);
	}

	void RHIWorldProxy::ReleaseRHI() {

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
	}

	World::World() {
		m_Proxy = new RHIWorldProxy();

		// FIXME make world an actual asset
		m_ID = 0;
		SafeRHIResourceInit(m_Proxy);
	}

	World::~World() {
		GFrameRenderer->SubmitToFrameQueue([proxy = m_Proxy]() {
			proxy->ReleaseRHI();
			delete proxy;
			});
	}

	Ref<World> World::Create() {
		Ref<World> w = CreateRef<World>();
		s_Current = w.Get();
		return w;
	}

	void World::Tick() {

		// tick the world
		{

		}
	}
}