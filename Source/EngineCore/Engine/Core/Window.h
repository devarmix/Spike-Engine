#pragma once

#include <Engine/Events/Event.h>

namespace Spike {

	struct WindowCreateInfo {

		std::string Name;

		uint32_t Width;
		uint32_t Height;
	};

	class Window {
	public:
		static Window* Create(const WindowCreateInfo& info);
		virtual ~Window() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual const std::string& GetName() const = 0;
		virtual void* GetNativeWindow() const = 0;

		virtual void OnUpdate() = 0;

		using EventCallbackFn = std::function<void(const GenericEvent&)>;
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	};
}
