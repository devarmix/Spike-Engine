#pragma once

#include <Engine/Core/Window.h>
#include <Engine/Events/ApplicationEvents.h>
#include <SDL.h>

namespace Spike {

	class WindowsWindow : public Window {
	public:
		WindowsWindow(const WindowCreateInfo& info);
		virtual ~WindowsWindow() override;

		virtual uint32_t GetWidth() const override { return m_Data.width; }
		virtual uint32_t GetHeight() const override { return m_Data.height; }

		virtual std::string GetName() const override { return m_Data.name; }
		virtual void* GetNativeWindow() const override { return m_Window; }

		virtual void OnUpdate() override;

		virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.eventCallback = callback; }

	private:
		struct WindowData {

			uint32_t width;
			uint32_t height;

			std::string name;
			EventCallbackFn eventCallback;
		};

		WindowData m_Data;
		SDL_Window* m_Window = nullptr;
	};
}
