#include <Engine/Layers/RenderLayer.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Renderer/RenderGraph.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/TextureBase.h>
#include <Engine/Core/Application.h>

#include <imgui_impl_sdl2.h>

namespace Spike {

	void RenderLayer::OnUpdate(float deltaTime) {

		EXECUTE_ON_RENDER_THREAD([]() {
			GRDGPool->FreeUnused();
			});
	}

	void RenderLayer::OnAttach() {

		EXECUTE_ON_RENDER_THREAD([]() {

			GShaderManager = new ShaderManager();
			GShaderManager->InitMaterialDataBuffer();

			GSamplerCache = new SamplerCache();
			GFrameRenderer = new FrameRenderer();
			GRDGPool = new RDGResourcePool();
			});
	}

	void RenderLayer::OnDetach() {

		EXECUTE_ON_RENDER_THREAD([]() {

			GRHIDevice->WaitGPUIdle();

			delete GRDGPool;
			delete GFrameRenderer;
			delete GSamplerCache;
			delete GShaderManager;
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