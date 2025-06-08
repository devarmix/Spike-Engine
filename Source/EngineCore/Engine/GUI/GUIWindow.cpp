#include <Engine/GUI/GUIWindow.h>
#include <Engine/Core/Application.h>

using namespace Spike;

namespace SpikeEngine {

	GUI_Window::GUI_Window(const std::string& name) : m_Name(name), m_WindowFlags(0) {}

	ImTextureID GUI_Window::MapGUITexture(Ref<Texture> texture) {

		return Application::Get()->GetImGUILayer()->GetGlobalGuiTextureMapper()->RegisterTexture(texture);
	}

	void GUI_Window::UpdateGUITexture(ImTextureID id, Ref<Texture> newTexture) {

		Application::Get()->GetImGUILayer()->GetGlobalGuiTextureMapper()->UpdateTexture(id, newTexture);
	}

	void GUI_Window::UnMapGUITexture(ImTextureID id) {

		Application::Get()->GetImGUILayer()->GetGlobalGuiTextureMapper()->UnregisterTexture(id);
	}
}