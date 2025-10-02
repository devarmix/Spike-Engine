#include <Engine/Layers/RenderLayer.h>
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Renderer/RenderGraph.h>
#include <Engine/Renderer/Shader.h>
#include <Engine/Renderer/TextureBase.h>
#include <Engine/Core/Application.h>

#include <imgui/imgui_impl_sdl2.h>
#include <Engine/Core/Log.h>

namespace Spike {

	void RenderLayer::Tick(float deltaTime) {

		SUBMIT_RENDER_COMMAND([]() {
			GRDGPool->FreeUnused();
			});
	}

	void RenderLayer::OnAttach() {
		SUBMIT_RENDER_COMMAND([]() {
			GShaderManager = new ShaderManager();
			GShaderManager->InitMatDataBuffer();

			GSamplerCache = new SamplerCache();
			GRDGPool = new RDGResourcePool();
			});

		GFrameRenderer = new FrameRenderer();
	}

	void RenderLayer::OnDetach() {

		SUBMIT_RENDER_COMMAND([]() {

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
				ImGui_ImplSDL2_ProcessEvent(&e.GetEvent());
				return false;
				});
		}
	}
}