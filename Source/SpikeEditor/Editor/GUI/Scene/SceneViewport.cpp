#include <Editor/GUI/Scene/SceneViewport.h>
#include <Engine/Spike.h>

#include "imgui_impl_vulkan.h"

#include <Editor/Core/Editor.h>

using namespace Spike;

namespace SpikeEditor {

	SceneViewport::~SceneViewport() {}

	void SceneViewport::OnCreate() {

		m_DSet = ImGui_ImplVulkan_AddTexture(VulkanRenderer::DefSamplerLinear, VulkanRenderer::ViewportTexture->GetRawData()->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VulkanRenderer::MainCamera = &m_Camera;

		SetWindowFlags(ImGuiWindowFlags_NoMove);
	}

	void SceneViewport::OnGUI(float deltaTime) {

		m_HoveredWindow = ImGui::IsWindowHovered();

		m_Camera.SetViewportHovered(m_HoveredWindow);

		// update camera
		m_Camera.OnUpdate(deltaTime);

		// poll events
		ImVec2 size = ImGui::GetContentRegionAvail();

		if (m_Width != size.x || m_Height != size.y) {

			// wait till gpu done its things, so we avoid any validation errors.
			vkDeviceWaitIdle(VulkanRenderer::Device.Device);

			if (!m_Minimized)
			    ImGui_ImplVulkan_RemoveTexture(m_DSet);

			if ((size.x <= 0 || size.y <= 0) && !m_Minimized) {

				// minimized window
				Spike::SceneViewportMinimizeEvent event;
				SpikeEditor::Get().OnEvent(event);

				m_Minimized = true;
			}

			if (m_Minimized && (size.x > 0 && size.y > 0)) {

				// restored window
				Spike::SceneViewportRestoreEvent event;
				SpikeEditor::Get().OnEvent(event);

				m_Minimized = false;
			}

			if (size.x < 0) m_Width = 0;
			else m_Width = (uint32_t)size.x;

			if (size.y < 0) m_Height = 0;
			else m_Height = (uint32_t)size.y;

			// resized window
			Spike::SceneViewportResizeEvent event(m_Width, m_Height);
			SpikeEditor::Get().OnEvent(event);

			if (!m_Minimized)
			    m_DSet = ImGui_ImplVulkan_AddTexture(VulkanRenderer::DefSamplerLinear, VulkanRenderer::ViewportTexture->GetRawData()->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		// draw viewport image
		if (!m_Minimized)
		    ImGui::Image((ImTextureID)m_DSet, size);
	}

	void SceneViewport::OnEvent(Spike::Event& event) {

		m_Camera.PollEvents(event);

		Spike::EventDispatcher dispatcher(event);
		dispatcher.Dispatch<Spike::MouseButtonPressEvent>(BIND_EVENT_FN(SceneViewport::OnMouseButtonPress));
	}

	bool SceneViewport::OnMouseButtonPress(const Spike::MouseButtonPressEvent& event) {

		if (event.GetButton() == SDL_BUTTON_RIGHT && m_HoveredWindow) {

			// if controlling camera, set this window as focused
			ImGui::SetWindowFocus(GetName().c_str());
		}

		return false;
	}
}