#pragma once

#include <Engine/Core/Layer.h>
//#include <Engine/Renderer/VK_Renderer.h>

#include <Engine/Events/ApplicationEvents.h>

#include <Engine/Renderer/Mesh.h>
using namespace SpikeEngine;

namespace Spike {

	class RendererLayer : public Layer {
	public:
		RendererLayer() : Layer("Renderer Layer") {}

		~RendererLayer() override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(const GenericEvent& event) override;

	private:
		bool OnWindowResize(const WindowResizeEvent& event);
		bool OnSDLEvent(const SDLEvent& event);
		
		bool OnSceneViewportResize(const SceneViewportResizeEvent& event);
		bool OnSceneViewportMinimize(const SceneViewportMinimizeEvent& event);
		bool OnSceneViewportRestore(const SceneViewportRestoreEvent& event);

		std::vector<Ref<Mesh>> Meshes;
	};
}
