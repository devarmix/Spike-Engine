#include <Engine/Renderer/RHIResource.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Renderer/FrameRenderer.h>

#include <Engine/Core/Application.h>

void Spike::SafeRHIResourceInit(RHIResource* resource) {

	if (resource) {

		SUBMIT_RENDER_COMMAND([resource]() {
			resource->InitRHI();
			});
	}
}

void Spike::SafeRHIResourceRelease(RHIResource* resource) {

	if (resource) {

		SUBMIT_RENDER_COMMAND([resource]() {

			resource->ReleaseRHI();
			delete resource;
			});
	}
}