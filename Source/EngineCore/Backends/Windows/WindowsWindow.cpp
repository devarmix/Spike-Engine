#include <Backends/Windows/WindowsWindow.h>

namespace Spike {

	WindowsWindow::WindowsWindow(const WindowDesc& desc) {

		m_Data.Width = desc.Width;
		m_Data.Height = desc.Height;
		m_Data.Name = desc.Name;

		// create a SDL window!
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
	}

	WindowsWindow::~WindowsWindow() {

		SDL_DestroyWindow(m_Window);
	}

	void WindowsWindow::OnUpdate() {

		// poll events;
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {

			SDLEvent s_event(e);
			m_Data.EventCallback(s_event);

			if (e.type == SDL_QUIT) {

				WindowCloseEvent event;
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_WINDOWEVENT) {

				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					
					WindowMinimizeEvent event;
					m_Data.EventCallback(event);
				}

				if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					
					WindowRestoreEvent event;
					m_Data.EventCallback(event);
				}

				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {

					int w, h;
					SDL_GetWindowSize(m_Window, &w, &h);

					m_Data.Width = w;
					m_Data.Height = h;

					WindowResizeEvent event(w, h);
					m_Data.EventCallback(event);
				}
			}

			if (e.type == SDL_KEYDOWN) {

				// check if key is repeated, if it is, dispatch its event
				if (e.key.repeat) {

					KeyRepeatEvent event(e.key.keysym.sym);
					m_Data.EventCallback(event);
				} else {

					KeyPressEvent event(e.key.keysym.sym);
					m_Data.EventCallback(event);
				}
			}

			if (e.type == SDL_KEYUP) {

				KeyReleaseEvent event(e.key.keysym.sym);
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_MOUSEBUTTONDOWN) {

				MouseButtonPressEvent event(e.button.button);
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_MOUSEBUTTONUP) {

				MouseButtonReleaseEvent event(e.button.button);
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_MOUSEMOTION) {

				float deltaX = (float)e.motion.xrel;
				float deltaY = (float)e.motion.yrel;

				MouseMotionEvent event(deltaX, deltaY);
				m_Data.EventCallback(event);
			}

			if (e.type == SDL_MOUSEWHEEL) {

				float scrollX = (float)e.wheel.x;
				float scrollY = (float)e.wheel.y;

				MouseScrollEvent event(scrollX, scrollY);
				m_Data.EventCallback(event);
			}
		}
	}
}