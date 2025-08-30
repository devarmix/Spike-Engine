#include <Engine/Layers/RenderLayer.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Renderer/RenderGraph.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/TextureBase.h>
#include <Engine/Core/Application.h>

#include <imgui_impl_sdl2.h>
#include <Engine/Core/Log.h>

namespace Spike {

	void RenderLayer::OnUpdate(float deltaTime) {

		EXECUTE_ON_RENDER_THREAD([]() {

			ENGINE_TRACE("UP");
			GRDGPool->FreeUnused();
			});
	}

	void RenderLayer::OnAttach() {

		EXECUTE_ON_RENDER_THREAD([]() {

			GShaderManager = new ShaderManager();
			GShaderManager->InitMatDataBuffer();

			GSamplerCache = new SamplerCache();
			GFrameRenderer = new FrameRenderer();
			GRDGPool = new RDGResourcePool();
			});
	}

	void RenderLayer::OnDetach() {

		EXECUTE_ON_RENDER_THREAD([]() {

			GRHIDevice->WaitGPUIdle();
			ENGINE_TRACE("DEL BG");

			delete GRDGPool;
			ENGINE_TRACE("DEL RDG Pool");
			delete GFrameRenderer;
			ENGINE_TRACE("DEL FRAME RENDR");
			delete GSamplerCache;
			ENGINE_TRACE("DEL SAMPLER CACHE");
			delete GShaderManager;

			ENGINE_TRACE("DEL END");
			});
	}

	void RenderLayer::OnEvent(const GenericEvent& event) {

		EventHandler handler(event);

		if (GApplication->IsUsingImGui()) {

			handler.Handle<SDLEvent>([this](const SDLEvent& e) {

				SDL_Event nativeEvent = e.GetEvent();

				EXECUTE_ON_RENDER_THREAD([nativeEvent]() {
					ImGui_ImplSDL2_ProcessEvent(&nativeEvent);
					});

				return false;
				});
		}
	}
}