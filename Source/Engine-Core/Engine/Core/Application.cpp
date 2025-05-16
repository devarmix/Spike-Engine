#include <Engine/Core/Application.h> 
#include <Engine/Core/Log.h>

#include <Engine/Core/Stats.h>

#include <Platforms/Vulkan/VulkanRenderer.h>

namespace Spike {

	Application::Application(const ApplicationCreateInfo& info) {

		m_Window = Window::Create(info.winInfo);
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		ENGINE_WARN("Created an application: " + info.name);

		// Initialize renderer
		VulkanRenderer::Init(*m_Window, RendererModeViewport);

		m_SceneLayer = new SceneLayer();
		m_ImGuiLayer = new ImGuiLayer();
		m_RendererLayer = new RendererLayer();

		// push default layers
		PushLayer(m_SceneLayer);

		PushOverlay(m_ImGuiLayer);
		PushOverlay(m_RendererLayer);

		m_ImGuiLayer->CreateGUI<StatsGUI>();

		m_Running = true;
	}

	void Application::CleanAll() {

		m_LayerStack.CleanAll();
	}


	void Application::Run() {

		while (m_Running) {

			// reset counters
			auto start = std::chrono::system_clock::now();

			if (!m_Minimized) {

				// update all layers
				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(m_Time.deltaTime);
				}
			}

			m_Window->OnUpdate();


			auto end = std::chrono::system_clock::now();

			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

			// convert from microseconds to seconds
			m_Time.deltaTime = elapsed.count() / 1000000.f;

			Stats::stats.frametime = elapsed.count() / 1000.f;
			Stats::stats.fps = 1000.f / Stats::stats.frametime;
		}
	}

	void Application::OnEvent(Event& event) {

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowMinimizeEvent>(BIND_EVENT_FN(Application::OnWindowMinimize));
		dispatcher.Dispatch<WindowRestoreEvent>(BIND_EVENT_FN(Application::OnWindowRestore));

		if (!event.IsHandled) {

			for (Layer* layer : m_LayerStack) {

				layer->OnEvent(event);
				if (event.IsHandled) break;
			}
		}
	}


	bool Application::OnWindowResize(const WindowResizeEvent& event) {

		ENGINE_WARN("Resized Window: Width {0}, Height {1}", event.GetWidth(), event.GetHeight());

		return false;
	}

	bool Application::OnWindowClose(const WindowCloseEvent& event) {

		m_Running = false;

		ENGINE_WARN("Closed Window");
		return false;
	}

	bool Application::OnWindowMinimize(const WindowMinimizeEvent& event) {

		m_Minimized = true;

		ENGINE_WARN("Minimized Window");
		return false;
	}

	bool Application::OnWindowRestore(const WindowRestoreEvent& event) {

		m_Minimized = false;

		ENGINE_WARN("Restored Window");
		return false;
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