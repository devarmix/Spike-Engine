#pragma once

#include <Engine/Core/Window.h>
#include <Engine/Events/ApplicationEvents.h>
#include <Engine/Core/LayerStack.h>

#include <Engine/Layers/RendererLayer.h>
#include <Engine/Layers/SceneLayer.h>
#include <Engine/Layers/ImGuiLayer.h>

#include <Engine/Core/Timestep.h>
#include <Engine/Core/Core.h>

namespace Spike {

	struct ApplicationCreateInfo {

		std::string name;
		WindowCreateInfo winInfo;
	};

	class Application {
	public:
		Application(const ApplicationCreateInfo& info);
		virtual ~Application() = default;

		// Layers
		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);

		void PushOverlay(Layer* layer);
		void PopOverlay(Layer* layer);

		template<typename T, typename... Args>
		Ref<T> CreateGUIWindow(Args&&... args) {

			return m_ImGuiLayer->CreateGUI<T>(std::forward<Args>(args)...);
		}

		//void DestroyGUIWindow();

		// entry point
		void Run();

		// events
		void OnEvent(Event& event);

		Ref<Window> GetMainWindow() const { return m_Window; }

		void CleanAll();

	private:
		bool OnWindowResize(const WindowResizeEvent& event);
		bool OnWindowClose(const WindowCloseEvent& event);
		bool OnWindowMinimize(const WindowMinimizeEvent& event);
		bool OnWindowRestore(const WindowRestoreEvent& event);

	private:
		Ref<Window> m_Window;
		LayerStack m_LayerStack;

		bool m_Running = false;
		bool m_Minimized = false;

		ImGuiLayer* m_ImGuiLayer;
		RendererLayer* m_RendererLayer;
		SceneLayer* m_SceneLayer;

		Time m_Time;
	};

	// to be defined in client
	Application* CreateApplication();
}