#pragma once

#include <Engine/Core/Core.h>

namespace Spike {

	class ResourceGPUData {
	public:

		virtual ~ResourceGPUData() = default;

		virtual void* GetNativeData() = 0;
	};

	class RenderResource {
	public:

		virtual ~RenderResource() = default;

		// executed only by render thread!
		virtual void InitGPUData() = 0;

		// executed only by render thread!
		virtual void ReleaseGPUData() = 0;
	};

	void SafeRenderResourceInit(RenderResource* resource);
	void SafeRenderResourceRelease(RenderResource* resource);
}