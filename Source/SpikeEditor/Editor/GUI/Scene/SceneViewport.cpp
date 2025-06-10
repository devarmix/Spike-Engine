#include <Editor/GUI/Scene/SceneViewport.h>
#include <Engine/Spike.h>

#include "imgui_impl_vulkan.h"

#include <Editor/Core/Editor.h>

using namespace Spike;

namespace SpikeEditor {

	SceneViewport::~SceneViewport() {

		m_ViewportTex.DestroyHandle();
	}

	void SceneViewport::OnCreate() {

		m_ViewportTex = GUITextureHandle(VulkanRenderer::ViewportTexture);
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
				m_ViewportTex.UpdateHandle(VulkanRenderer::ViewportTexture);
		}

		// draw viewport image
		if (!m_Minimized)
		    ImGui::Image(m_ViewportTex, size);
	}

	void SceneViewport::OnEvent(const Spike::GenericEvent& event) {

		m_Camera.PollEvents(event);

		Spike::EventHandler handler(event);
		handler.Handle<Spike::MouseButtonPressEvent>(BIND_FUNCTION(SceneViewport::OnMouseButtonPress));
	}

	bool SceneViewport::OnMouseButtonPress(const Spike::MouseButtonPressEvent& event) {

		if (event.GetButton() == SDL_BUTTON_RIGHT && m_HoveredWindow) {

			// if controlling camera, set this window as focused
			ImGui::SetWindowFocus(GetName().c_str());
		}

		return false;
	}
}