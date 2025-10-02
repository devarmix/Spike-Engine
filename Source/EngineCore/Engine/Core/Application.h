#pragma once

#include <Engine/Core/Window.h>
#include <Engine/Renderer/GfxDevice.h>
#include <Engine/Events/ApplicationEvents.h>
#include <Engine/Core/LayerStack.h>
#include <Engine/Layers/RenderLayer.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Core/Core.h>

namespace Spike {

	struct ApplicationDesc {

		std::string Name;
		bool UsingImGui;
		bool UsingDocking;
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
		void Tick();

		// events
		void EnqueueEvent(std::function<void()>&& event) {
			m_EventQueue.Push(std::move(event));
		}

		template<typename T, typename... Args, bool DispatchImmediate = false>
		void DispatchEvent(Args&&... args) {

			std::shared_ptr<T> event = std::make_shared<T>(std::forward<Args>(args)...);
			if constexpr (DispatchImmediate) {
				OnEvent(*event);
			}
			else {
				EnqueueEvent([event, this]() {
					OnEvent(*event);
					});
			}
		}

		Window* GetMainWindow() { return m_Window; }
		RenderLayer* GetRenderLayer() { return m_RenderLayer; }
		RenderThread& GetRenderThread() { return m_RenderThread; }

		bool IsUsingImGui() const { return m_UsingImGui; }
		bool IsUsingDocking() const { return m_UsingDocking; }
		void Destroy();

	private:

		void ProcessEvents();
		void OnEvent(const GenericEvent& event);

	private:
		Window* m_Window;

		LayerStack m_LayerStack;
		RenderThread m_RenderThread;

		ThreadSafeQueue<std::function<void()>> m_EventQueue;
		//ThreadSafeQueue<std::function<void()>> m_DeletorsQueue;

		bool m_Running = false;
		bool m_Minimized = false;
		bool m_UsingImGui = false;
		bool m_UsingDocking = false;

		RenderLayer* m_RenderLayer;

		Time m_Time;
	};

	// to be defined in client
	Application* CreateApplication(int argc, char* argv[]);

	// global application pointer
	extern Application* GApplication;
}

#define SUBMIT_RENDER_COMMAND(task) Spike::GApplication->GetRenderThread().PushTask(task);