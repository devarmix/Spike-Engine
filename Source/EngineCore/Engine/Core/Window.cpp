#include <Engine/Core/Window.h>

#ifdef ENGINE_PLATFORM_WINDOWS
#include <Backends/Windows/WindowsWindow.h>
#endif 

Spike::InputHandler* Spike::GInput = nullptr;

namespace Spike {

	Window* Window::Create(const WindowDesc& desc) {

#ifdef ENGINE_PLATFORM_WINDOWS
		return new WindowsWindow(desc);
#else
#error Spike Engine supports only Windows!
		return nullptr;
#endif 

	}

	void InputHandler::OnEvent(const SDL_Event& event) {

		switch (event.type)
		{
		case SDL_KEYDOWN:
			m_States[(EKey)event.key.keysym.scancode] = { true, true, false };
			m_Pressed.push_back((EKey)event.key.keysym.scancode);
			break;
		case SDL_KEYUP:
			m_States[(EKey)event.key.keysym.scancode] = { false, false, true };
			m_Pressed.push_back((EKey)event.key.keysym.scancode);
			break;
		case SDL_MOUSEMOTION:
			m_DeltaMouseX = event.motion.xrel;
			m_DeltaMouseY = event.motion.yrel;

			m_MouseX = event.motion.x;
			m_MouseY = event.motion.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			m_MouseButtons[event.button.button - 1] = { true, true, false };
			break;
		case SDL_MOUSEBUTTONUP:
			m_MouseButtons[event.button.button - 1] = { false, false, true };
			break;
		case SDL_MOUSEWHEEL:
			m_MouseScroll = event.wheel.y;
			break;
		default:
			break;
		}
	}

	void InputHandler::Tick() {

		for (auto key : m_Pressed) {
			m_States[key].Pressed = false;
		}

		for (auto key : m_Released) {
			m_States[key].Released = false;
		}

		for (int i = 0; i < 3; i++) {
			m_MouseButtons[i].Pressed = false;
			m_MouseButtons[i].Released = false;
		}

		m_DeltaMouseX = 0;
		m_DeltaMouseY = 0;
		m_MouseScroll = 0;

		m_Released.clear();
		m_Pressed.clear();
	}
}