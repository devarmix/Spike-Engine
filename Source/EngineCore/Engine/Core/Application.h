#pragma once

#include <Engine/Core/Window.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Events/ApplicationEvents.h>
#include <Engine/Core/LayerStack.h>
#include <Engine/Layers/SceneLayer.h>
#include <Engine/Layers/RenderLayer.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Core/Core.h>

namespace Spike {

	struct ApplicationDesc {

		std::string Name;
		bool UsingImGui;
		WindowDesc WindowDesc;
	};

	class Application {
	public:
		Application(const ApplicationDesc& info);
		virtual ~Application() = default;

		// Layers
		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);

		void PushOverlay(Layer* layer);
		void PopOverlay(Layer* layer);
		// entry point
		void Run();

		// events
		void OnEvent(const GenericEvent& event);

		Window* GetMainWindow() { return m_Window; }
		RenderLayer* GetRenderLayer() { return m_RenderLayer; }
		RenderThread& GetRenderThread() { return m_RenderThread; }

		bool IsUsingImGui() const { return m_UsingImGui; }
		void Destroy();

	private:

		Window* m_Window;

		LayerStack m_LayerStack;
		RenderThread m_RenderThread;

		//std::vector<GenericEvent> m_EventQueue;
		//std::mutex m_EventQueueMutex;

		//TaskScheduler m_TaskScheduler;
		//SyncStructures m_SyncStructures;

		bool m_Running = false;
		bool m_Minimized = false;
		bool m_UsingImGui = false;

		SceneLayer* m_SceneLayer;
		RenderLayer* m_RenderLayer;

		Time m_Time;
	};

	// to be defined in client
	Application* CreateApplication();

	// global application pointer
	extern Application* GApplication;
}

#define EXECUTE_ON_RENDER_THREAD(task) Spike::GApplication->GetRenderThread().PushTask(std::move(task));