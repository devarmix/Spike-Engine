#pragma once

#include <Engine/Core/Layer.h>
#include <Engine/GUI/GUIWindow.h>
#include <Engine/Renderer/Texture.h>

using namespace SpikeEngine;

namespace Spike {

	class ImGuiTextureMapper {
	public:
		virtual ~ImGuiTextureMapper() = default;

		static ImGuiTextureMapper* Create();

		virtual ImTextureID RegisterTexture(Ref<Texture> texture) = 0;
		virtual void UnregisterTexture(ImTextureID id) = 0;

		virtual void UpdateTexture(ImTextureID id, Ref<Texture> newTexture) = 0;
	};

	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer() : Layer("ImGui Layer") { m_WindowList.clear(); 
		    m_GlobalGuiTextureMapper = ImGuiTextureMapper::Create(); }
		~ImGuiLayer() override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(const GenericEvent& event) override;

		template<typename T, typename... Args>
		Ref<T> CreateGUI(Args&&... args) {

			Ref<T> window = CreateRef<T>(std::forward<Args>(args)...);
			window->OnCreate();
			m_WindowList.push_back(window);

			return window;
		}

		ImGuiTextureMapper* GetGlobalGuiTextureMapper() { return m_GlobalGuiTextureMapper; }

		//static void DestroyWindow(Ref<GUI_Window>& window);

	private:
		void SetDarkColors();

	private:
		std::vector<Ref<GUI_Window>> m_WindowList;
		ImGuiTextureMapper* m_GlobalGuiTextureMapper;
	};
}