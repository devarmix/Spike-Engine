#include <Engine/Renderer/RenderResource.h>
#include <Engine/Renderer/GfxDevice.h>

#include <Engine/Core/Application.h>

void Spike::SafeRenderResourceInit(RenderResource* resource) {

	EXECUTE_ON_RENDER_THREAD([resource]() {
		
		if (resource) {

			resource->InitGPUData();
		}
		});
}

void Spike::SafeRenderResourceRelease(RenderResource* resource) {

	EXECUTE_ON_RENDER_THREAD([resource]() {

		if (resource) {

			resource->ReleaseGPUData();
			delete resource;
		}
		});
}