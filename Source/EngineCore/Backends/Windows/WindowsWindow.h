#pragma once

#include <Engine/Core/Window.h>
#include <Engine/Events/ApplicationEvents.h>
#include <SDL.h>

namespace Spike {

	class WindowsWindow : public Window {
	public:
		WindowsWindow(const WindowDesc& desc);
		virtual ~WindowsWindow() override;

		virtual uint32_t GetWidth() const override { return m_Data.Width; }
		virtual uint32_t GetHeight() const override { return m_Data.Height; }

		virtual const std::string& GetName() const override { return m_Data.Name; }
		virtual void* GetNativeWindow() const override { return m_Window; }

		virtual void OnUpdate() override;

		virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

	private:

		struct WindowData {

			uint32_t Width;
			uint32_t Height;

			std::string Name;
			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		SDL_Window* m_Window = nullptr;
	};
}
