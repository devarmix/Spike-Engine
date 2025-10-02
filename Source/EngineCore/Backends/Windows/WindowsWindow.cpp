#include <Backends/Windows/WindowsWindow.h>

namespace Spike {

	WindowsWindow::WindowsWindow(const WindowDesc& desc) {
		m_Data.Width = desc.Width;
		m_Data.Height = desc.Height;
		m_Data.Name = desc.Name;

		SDL_Init(SDL_INIT_VIDEO);
		SDL_WindowFlags window_Flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		m_Window = SDL_CreateWindow(
			desc.Name.c_str(),
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			desc.Width,
			desc.Height,
			window_Flags
		);

		GInput = new InputHandler();
	}

	WindowsWindow::~WindowsWindow() {
		SDL_DestroyWindow(m_Window);
		delete GInput;
	}

	void WindowsWindow::Tick() {
		GInput->Tick();

		// poll events
		SDL_Event e{};
		while (SDL_PollEvent(&e) != 0) {

			{
				SDLEvent event(e);
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_QUIT) {
				WindowCloseEvent event{};
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					WindowMinimizeEvent event{};
					m_Data.EventCallback(event);
				}
				else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					WindowRestoreEvent event{};
					m_Data.EventCallback(event);
				}
				else if (e.window.event == SDL_WINDOWEVENT_RESIZED) {

					int w, h;
					SDL_GetWindowSize(m_Window, &w, &h);

					m_Data.Width = w;
					m_Data.Height = h;

					WindowResizeEvent event(w, h);
					m_Data.EventCallback(event);
				}
			}

			GInput->OnEvent(e);
		}
	}
}