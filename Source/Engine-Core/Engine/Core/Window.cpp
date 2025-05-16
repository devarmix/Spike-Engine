#include <Engine/Core/Window.h>

#ifdef ENGINE_PLATFORM_WINDOWS
#include <Platforms/Windows/WindowsWindow.h>
#endif 


namespace Spike {

	Ref<Window> Window::Create(const WindowCreateInfo& info) {

#ifdef ENGINE_PLATFORM_WINDOWS
		return CreateRef<WindowsWindow>(info);
#else
#error Spike Engine supports only Windows!
		return nullptr;
#endif 

	}
}