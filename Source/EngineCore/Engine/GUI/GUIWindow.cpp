#include <Engine/GUI/GUIWindow.h>
#include <Engine/Core/Application.h>
#include <Engine/Core/Log.h>

using namespace Spike;

namespace SpikeEngine {

	GUIWindow::GUIWindow(const std::string& name) : m_Name(name), m_WindowFlags(0) {}


	GUITextureHandle::GUITextureHandle() : m_Id(0), m_Texture(nullptr) {}

	GUITextureHandle::~GUITextureHandle() {}

	GUITextureHandle::GUITextureHandle(Ref<Texture> texture) {

		m_Id = Application::Get().GetImGUILayer()->GetGlobalGuiTextureMapper()->RegisterTexture(texture);
		m_Texture = texture;
	}

	void GUITextureHandle::DestroyHandle() {

		Application::Get().GetImGUILayer()->GetGlobalGuiTextureMapper()->UnregisterTexture(m_Id);
	}

	void GUITextureHandle::UpdateHandle(Ref<Texture> newTexture) {

		Application::Get().GetImGUILayer()->GetGlobalGuiTextureMapper()->UpdateTexture(m_Id, newTexture);
		m_Texture = newTexture;
	}
}