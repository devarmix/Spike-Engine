#pragma once

#include <Engine/SpikeEngine.h>
using namespace SpikeEngine;

#include <Editor/Renderer/EditorCamera.h>
#include <Platforms/Vulkan/VulkanRenderer.h>

namespace SpikeEditor {

	class SceneViewport : public GUI_Window {
	public:
		SceneViewport() : GUI_Window("Scene") {}
		~SceneViewport() override;

		virtual void OnCreate() override;
		virtual void OnGUI(float deltaTime) override;

		virtual void OnEvent(Spike::Event& event) override;

	private:

		bool OnMouseButtonPress(const Spike::MouseButtonPressEvent& event);

	private:
		uint32_t m_Width = 100;
		uint32_t m_Height = 100;

		VkDescriptorSet m_DSet;

		bool m_Minimized = false;

		EditorCamera m_Camera;
		bool m_HoveredWindow = false;
	};
}
