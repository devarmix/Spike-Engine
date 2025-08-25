#include <Engine/Core/Window.h>

#ifdef ENGINE_PLATFORM_WINDOWS
#include <Backends/Windows/WindowsWindow.h>
#endif 


namespace Spike {

	Window* Window::Create(const WindowDesc& desc) {

#ifdef ENGINE_PLATFORM_WINDOWS
		return new WindowsWindow(desc);
#else
#error Spike Engine supports only Windows!
		return nullptr;
#endif 

	}
}