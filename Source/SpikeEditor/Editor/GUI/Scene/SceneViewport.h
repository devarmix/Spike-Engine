#pragma once

#include <Engine/SpikeEngine.h>
using namespace SpikeEngine;

#include <Editor/Renderer/EditorCamera.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

namespace SpikeEditor {

	class SceneViewport : public GUIWindow {
	public:
		SceneViewport() : GUIWindow("Scene") {}
		virtual ~SceneViewport() override;

		virtual void OnCreate() override;
		virtual void OnGUI(float deltaTime) override;

		virtual void OnEvent(const Spike::GenericEvent& event) override;

	private:

		bool OnMouseButtonPress(const Spike::MouseButtonPressEvent& event);

	private:
		uint32_t m_Width = 100;
		uint32_t m_Height = 100;

		GUITextureHandle m_ViewportTex;

		bool m_Minimized = false;

		EditorCamera m_Camera;
		bool m_HoveredWindow = false;
	};
}
