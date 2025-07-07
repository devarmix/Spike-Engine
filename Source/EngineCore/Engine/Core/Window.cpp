#include <Engine/Core/Window.h>

#ifdef ENGINE_PLATFORM_WINDOWS
#include <Platforms/Windows/WindowsWindow.h>
#endif 


namespace Spike {

	Window* Window::Create(const WindowCreateInfo& info) {

#ifdef ENGINE_PLATFORM_WINDOWS
		return new WindowsWindow(info);
#else
#error Spike Engine supports only Windows!
		return nullptr;
#endif 

	}
}