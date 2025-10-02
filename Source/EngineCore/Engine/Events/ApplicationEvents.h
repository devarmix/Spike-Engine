#pragma once

#include <Engine/Events/Event.h>
#include <sdl2/SDL_events.h>

namespace Spike {

	class WindowResizeEvent : public GenericEvent {
	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

		unsigned int GetWidth() const { return m_Width; }
		unsigned int GetHeight() const { return m_Height; }

		std::string GetName() const override { return "Window Resize Event"; }

		EVENT_CLASS_TYPE(EWindowResize);

	private:
		unsigned int m_Width;
		unsigned int m_Height;
	};

	class WindowCloseEvent : public GenericEvent {
	public:
		WindowCloseEvent() {}
		std::string GetName() const override { return "Window Close Event"; }

		EVENT_CLASS_TYPE(EWindowClose);
	};

	class WindowMinimizeEvent : public GenericEvent {
	public:
		WindowMinimizeEvent() {}
		std::string GetName() const override { return "Window Minimize Event"; }

		EVENT_CLASS_TYPE(EWindowMinimize);
	};

	class WindowRestoreEvent : public GenericEvent {
	public:
		WindowRestoreEvent() {}
		std::string GetName() const override { return "Window Restore Event"; }

		EVENT_CLASS_TYPE(EWindowRestore);
	};

	class SDLEvent : public GenericEvent {
	public:
		SDLEvent(const SDL_Event& e) : event(e) {}
		std::string GetName() const override { return "SDL Event"; }

		const SDL_Event& GetEvent() const { return event; }

		EVENT_CLASS_TYPE(ESDL);

	private:
		SDL_Event event;
	};
}
