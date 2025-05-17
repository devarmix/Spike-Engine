#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Events/Event.h>
#include <imgui.h>

namespace SpikeEngine {

	class GUI_Window {
	public:
		GUI_Window(const std::string& name = "New Window");
		virtual ~GUI_Window() = default;

		virtual void OnCreate() = 0;
		virtual void OnGUI(float deltaTime) = 0;
		virtual void OnEvent(Spike::Event& event) {}

		const std::string& GetName() const { return m_Name; }

		const ImGuiWindowFlags GetWindowFlags() const { return m_WindowFlags; }
		void SetWindowFlags(ImGuiWindowFlags flags) { m_WindowFlags = flags; }
		void AddWindowFlags(ImGuiWindowFlags flags) { m_WindowFlags |= flags; }

	private:
		std::string m_Name;

		ImGuiWindowFlags m_WindowFlags;
	};
}
