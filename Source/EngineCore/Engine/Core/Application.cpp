#include <Engine/Core/Application.h> 
#include <Engine/Core/Log.h>

#include <Engine/Core/Stats.h>
#include "imgui_impl_sdl2.h"

Spike::Application* Spike::GApplication = nullptr;

namespace Spike {

	Application::Application(const ApplicationCreateInfo& info) {

		m_Window = Window::Create(info.WinInfo);
		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));

		//m_TaskScheduler.Init(5);
		m_UsingImGui = info.UsingImGui;

		// initialize globals
		GApplication = this;
		//GTaskScheduler = &m_TaskScheduler;
		GfxDevice::Create(m_Window, m_UsingImGui);

		ENGINE_WARN("Created an application: " + info.Name);

		m_SceneLayer = new SceneLayer();

		// push default layers
		PushLayer(m_SceneLayer);

		m_Running = true;
	}

	void Application::Destroy() {

		m_LayerStack.CleanAll();

		EXECUTE_ON_RENDER_THREAD([]() {

			delete GGfxDevice;
			});

		m_RenderThread.Terminate();
		m_RenderThread.Join();

		delete m_Window;
	}


	void Application::Run() {

		while (m_Running) {

			Timer timer = Timer();

			m_RenderThread.WaitTillDone();

			// process queued events when render thread is idle
			//ProcessEvents();

			// process last frame
			m_RenderThread.Process();

			EXECUTE_ON_RENDER_THREAD([]() {

				GGfxDevice->BeginFrameCommandRecording();
				});

			if (!m_Minimized) {

				// update all layers
				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(m_Time.DeltaTime);
				}
			}

			m_Window->OnUpdate();

			uint32_t width = m_Window->GetWidth();
			uint32_t height = m_Window->GetHeight();

			EXECUTE_ON_RENDER_THREAD([=]() {

				GGfxDevice->DrawSwapchain(width, height, true);
				});

			// sync render thread
			//m_SyncStructures.FrameEndSync();

			float elapsed = timer.GetElapsedMs();

			m_Time.DeltaTime = elapsed / 1000.f;

			Stats::Data.Frametime = elapsed;
			Stats::Data.Fps = 1000.f / Stats::Data.Frametime;
		}
	}

	void Application::OnEvent(const GenericEvent& event) {

		EventHandler handler(event);

		handler.Handle<WindowCloseEvent>([this](const WindowCloseEvent& e) {

			m_Running = false;

		    ENGINE_WARN("Closed Window");
		    return false;
			});

		handler.Handle<WindowMinimizeEvent>([this](const WindowMinimizeEvent& e) {

			m_Minimized = true;

			ENGINE_WARN("Minimized Window");
			return true;
			});

		handler.Handle<WindowRestoreEvent>([this](const WindowRestoreEvent& e) {

			m_Minimized = false;

			ENGINE_WARN("Restored Window");
			return true;
			});

		handler.Handle<SDLEvent>([this](const SDLEvent& e) {

			SDL_Event nativeEvent = e.GetEvent();

			if (m_UsingImGui) {

				// process imgui events on render thread
				// TODO: probably move this to a dedicated ImGui layer or smth
				EXECUTE_ON_RENDER_THREAD([nativeEvent]() {

					ImGui_ImplSDL2_ProcessEvent(&nativeEvent);
					});
			}

			return false;
			});

		if (!event.IsHandled()) {

			for (Layer* layer : m_LayerStack) {

				layer->OnEvent(event);
				if (event.IsHandled()) break;
			}
		}
	}

	void Application::PushLayer(Layer* layer) {
		m_LayerStack.PushLayer(layer);
	}

	void Application::PopLayer(Layer* layer) {
		m_LayerStack.PopLayer(layer);
	}


	void Application::PushOverlay(Layer* overlay) {
		m_LayerStack.PushOverlay(overlay);
	}

	void Application::PopOverlay(Layer* overlay) {
		m_LayerStack.PopOverlay(overlay);
	}
}