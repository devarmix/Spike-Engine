#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Events/Event.h>
#include <Engine/Renderer/Texture.h>
#include <imgui.h>

namespace SpikeEngine {

	class GUIWindow {
	public:
		GUIWindow(const std::string& name);
		virtual ~GUIWindow() = default;

		virtual void OnCreate() = 0;
		virtual void OnGUI(float deltaTime) = 0;
		virtual void OnEvent(const Spike::GenericEvent& event) {}

		const std::string& GetName() const { return m_Name; }

		ImGuiWindowFlags GetWindowFlags() const { return m_WindowFlags; }
		void SetWindowFlags(ImGuiWindowFlags flags) { m_WindowFlags = flags; }
		void AddWindowFlags(ImGuiWindowFlags flags) { m_WindowFlags |= flags; }

	private:
		std::string m_Name;

		ImGuiWindowFlags m_WindowFlags;
	};

	class GUITextureHandle {
	public:
		GUITextureHandle();
		GUITextureHandle(Ref<Texture> texture);

		~GUITextureHandle();
		void DestroyHandle();

		void UpdateHandle(Ref<Texture> newTexture);
		Ref<Texture> GetRawTexture() const { return m_Texture; }

		operator ImTextureID() const { return m_Id; }

	private:
		Ref<Texture> m_Texture;
		ImTextureID m_Id;
	};
}
