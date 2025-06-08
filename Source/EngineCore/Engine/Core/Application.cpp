#include <Engine/Core/Application.h> 
#include <Engine/Core/Log.h>

#include <Engine/Core/Stats.h>

#include <Platforms/Vulkan/VulkanRenderer.h>

namespace Spike {

	Application* Application::m_Instance = nullptr;

	Application::Application(const ApplicationCreateInfo& info) {

		assert(m_Instance == nullptr);
		m_Instance = this;

		m_Window = Window::Create(info.WinInfo);
		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));

		ENGINE_WARN("Created an application: " + info.Name);

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

			Timer timer = Timer();

			if (!m_Minimized) {

				// update all layers
				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(m_Time.DeltaTime);
				}
			}

			m_Window->OnUpdate();

			float elapsed = timer.GetElapsedMs();

			m_Time.DeltaTime = elapsed / 1000.f;

			Stats::Data.Frametime = elapsed;
			Stats::Data.Fps = 1000.f / Stats::Data.Frametime;
		}
	}

	void Application::OnEvent(const GenericEvent& event) {

		EventHandler handler(event);
		handler.Handle<WindowResizeEvent>(BIND_FUNCTION(Application::OnWindowResize));
		handler.Handle<WindowCloseEvent>(BIND_FUNCTION(Application::OnWindowClose));
		handler.Handle<WindowMinimizeEvent>(BIND_FUNCTION(Application::OnWindowMinimize));
		handler.Handle<WindowRestoreEvent>(BIND_FUNCTION(Application::OnWindowRestore));

		if (!event.IsHandled()) {

			for (Layer* layer : m_LayerStack) {

				layer->OnEvent(event);
				if (event.IsHandled()) break;
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