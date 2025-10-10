#include <Engine/Core/Application.h> 
#include <Engine/Renderer/FrameRenderer.h>
#include <Engine/Core/Log.h>

#include <Engine/Core/Stats.h>

Spike::Application* Spike::Application::s_Instance = nullptr;

namespace Spike {

	Application::Application(const ApplicationDesc& desc) {

		m_Window = Window::Create(desc.WindowDesc);
		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));

		m_UsingImGui = desc.UsingImGui;
		m_UsingDocking = desc.UsingDocking;

		// initialize core globals
		s_Instance = this;
		RHIDevice::Create(m_Window, m_UsingImGui);

		ENGINE_WARN("Created an application: " + desc.Name);

		m_RenderLayer = new RenderLayer();
		PushOverlay(m_RenderLayer);

		m_Running = true;
	}

	void Application::Destroy() {

		m_LayerStack.CleanAll();

		SUBMIT_RENDER_COMMAND([]() {
			delete GRHIDevice;
			});

		m_RenderThread.Terminate();
		m_RenderThread.Join();

		delete m_Window;
	}


	void Application::Tick() {

		while (m_Running) {
			Timer timer = Timer();

			m_RenderThread.WaitTillDone();
			ProcessEvents();

			m_RenderThread.Process();
			GFrameRenderer->BeginFrame();

			if (!m_Minimized) {

				// update all layers
				for (Layer* layer : m_LayerStack) {
					layer->Tick(m_Time.DeltaTime);
				}
			}

			m_Window->Tick();
			GFrameRenderer->RenderSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

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

		if (!event.IsHandled()) {

			for (Layer* layer : m_LayerStack) {
				layer->OnEvent(event);
				if (event.IsHandled()) break;
			}
		}
	}

	void Application::ProcessEvents() {
		std::vector<std::function<void()>> queue{};

		m_EventQueue.PopAll(queue);
		for (auto& func : queue) {
			func();
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