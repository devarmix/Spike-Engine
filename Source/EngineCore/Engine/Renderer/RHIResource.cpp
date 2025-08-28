#include <Engine/Renderer/RHIResource.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/FrameRenderer.h>

#include <Engine/Core/Application.h>

void Spike::SafeRHIResourceInit(RHIResource* resource) {

	EXECUTE_ON_RENDER_THREAD([resource]() {

		if (resource) {

			resource->InitRHI();
		}
		});
}

void Spike::SafeRHIResourceRelease(RHIResource* resource) {

	EXECUTE_ON_RENDER_THREAD([resource]() {

		if (resource) {

			resource->ReleaseRHI();
			delete resource;
		}
		});
}