#pragma once

#include <Engine/Events/Event.h>
#include <SDL.h>

namespace Spike {

	class WindowResizeEvent : public GenericEvent {
	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

		unsigned int GetWidth() const { return m_Width; }
		unsigned int GetHeight() const { return m_Height; }

		std::string GetName() const override { return "Window Resize Event"; }

		EVENT_CLASS_TYPE(WindowResize)

	private:
		unsigned int m_Width;
		unsigned int m_Height;
	};

	class WindowCloseEvent : public GenericEvent {
	public:
		WindowCloseEvent() {}
		std::string GetName() const override { return "Window Close Event"; }

		EVENT_CLASS_TYPE(WindowClose)
	};

	class WindowMinimizeEvent : public GenericEvent {
	public:
		WindowMinimizeEvent() {}
		std::string GetName() const override { return "Window Minimize Event"; }

		EVENT_CLASS_TYPE(WindowMinimize)
	};

	class WindowRestoreEvent : public GenericEvent {
	public:
		WindowRestoreEvent() {}
		std::string GetName() const override { return "Window Restore Event"; }

		EVENT_CLASS_TYPE(WindowRestore)
	};

	class SDLEvent : public GenericEvent {
	public:
		SDLEvent(SDL_Event& e) : event(e) {}
		std::string GetName() const override { return "SDL Event"; }

		const SDL_Event& GetEvent() const { return event; }

		EVENT_CLASS_TYPE(SDL)

	private:
		SDL_Event event;
	};

	class SceneViewportResizeEvent : public GenericEvent {
	public:
		SceneViewportResizeEvent(uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		std::string GetName() const override { return "Scene Viewport Resize Event"; }

		EVENT_CLASS_TYPE(SceneViewportResize)

	private:
		uint32_t m_Width;
		uint32_t m_Height;
	};

	class SceneViewportMinimizeEvent : public GenericEvent {
	public:
		SceneViewportMinimizeEvent() {}
		std::string GetName() const override { return "Scene Viewport Minimize Event"; }

		EVENT_CLASS_TYPE(SceneViewportMinimize)
	};

	class SceneViewportRestoreEvent : public GenericEvent {
	public:
		SceneViewportRestoreEvent() {}
		std::string GetName() const override { return "Scene Viewport Restore Event"; }

		EVENT_CLASS_TYPE(SceneViewportRestore)
	};

	class KeyPressEvent : public GenericEvent {
	public:
		KeyPressEvent(SDL_Keycode key) : m_Key(key) {}
		std::string GetName() const override { return "Key Press Event"; }

		SDL_Keycode GetKey() const { return m_Key; }

		EVENT_CLASS_TYPE(KeyPress)

	private:
		SDL_Keycode m_Key;
	};

	class KeyReleaseEvent : public GenericEvent {
	public:
		KeyReleaseEvent(SDL_Keycode key) : m_Key(key) {}
		std::string GetName() const override { return "Key Release Event"; }

		SDL_Keycode GetKey() const { return m_Key; }

		EVENT_CLASS_TYPE(KeyRelease)

	private:
		SDL_Keycode m_Key;
	};

	class KeyRepeatEvent : public GenericEvent {
	public:
		KeyRepeatEvent(SDL_Keycode key) : m_Key(key) {}
		std::string GetName() const override { return "Key Repeat Event"; }

		SDL_Keycode GetKey() const { return m_Key; }

		EVENT_CLASS_TYPE(KeyRepeat)

	private:
		SDL_Keycode m_Key;
	};

	class MouseButtonPressEvent : public GenericEvent {
	public:
		MouseButtonPressEvent(Uint8 button) : m_Button(button) {}
		std::string GetName() const override { return "Mouse Button Press Event"; }

		Uint8 GetButton() const { return m_Button; }

		EVENT_CLASS_TYPE(MouseButtonPress)

	private:
		Uint8 m_Button;
	};

	class MouseButtonReleaseEvent : public GenericEvent {
	public:
		MouseButtonReleaseEvent(Uint8 button) : m_Button(button) {}
		std::string GetName() const override { return "Mouse Button Release Event"; }

		Uint8 GetButton() const { return m_Button; }

		EVENT_CLASS_TYPE(MouseButtonRelease)

	private:
		Uint8 m_Button;
	};

	class MouseMotionEvent : public GenericEvent {
	public:
		MouseMotionEvent(float deltaX, float deltaY) : m_MouseDeltaX(deltaX), m_MouseDeltaY(deltaY) {}
		std::string GetName() const override { return "Mouse Motion Event"; }

		float GetDeltaX() const { return m_MouseDeltaX; }
		float GetDeltaY() const { return m_MouseDeltaY; }

		EVENT_CLASS_TYPE(MouseMotion)

	private:
		float m_MouseDeltaX;
		float m_MouseDeltaY;
	};
	
	class MouseScrollEvent : public GenericEvent {
	public:
		MouseScrollEvent(float x, float y) : m_ScrollX(x), m_ScrollY(y) {}
		std::string GetName() const override { return "Mouse Scroll Event"; }

		float GetScrollX() const { return m_ScrollX; }
		float GetScrollY() const { return m_ScrollY; }

		EVENT_CLASS_TYPE(MouseScroll)

	private:
		float m_ScrollX;
		float m_ScrollY;
	};
}
