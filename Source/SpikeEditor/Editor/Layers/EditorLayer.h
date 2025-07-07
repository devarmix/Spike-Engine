#pragma once

#include <Engine/Core/Layer.h>
#include <Engine/Renderer/GfxDevice.h>
using namespace Spike;

#include <Editor/Renderer/EditorCamera.h>
#include <imgui.h>

namespace SpikeEditor {

	class EditorLayer : public Layer {
	public:
		EditorLayer() : Layer("Editor Layer") {}
		~EditorLayer() override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(const GenericEvent& event) override;

	private:

		bool OnMouseButtonPress(const MouseButtonPressEvent& event);

	private:

		EditorCamera m_EditorCamera;

		Ref<Texture2D> m_SceneViewport;
		GBufferResource* m_GBuffer = nullptr;

		Ref<CubeTexture> m_IrradianceTexture;
		Ref<CubeTexture> m_EnvironmentTexture;
		Ref<CubeTexture> m_SkyboxTexture;

		std::vector<Ref<Mesh>> m_LoadedMeshes;

		bool m_ViewportHovered = false;
		bool m_ViewportMinimized = false;

		uint32_t m_Width = 1920;
		uint32_t m_Height = 1080;
	};
}
