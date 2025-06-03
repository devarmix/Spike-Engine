#pragma once

#include <Engine/Core/Layer.h>
#include <Engine/GUI/GUIWindow.h>
using namespace SpikeEngine;

namespace Spike {

	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer() : Layer("ImGui Layer") {}
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

		//static void DestroyWindow(Ref<GUI_Window>& window);

	private:
		void SetDarkColors();

	private:
		std::vector<Ref<GUI_Window>> m_WindowList;
	};
}