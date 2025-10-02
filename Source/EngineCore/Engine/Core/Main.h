#pragma once

#include <Engine/Core/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Log.h>

#ifdef ENGINE_PLATFORM_WINDOWS
extern Spike::Application* Spike::CreateApplication(int argc, char* argv[]);

int main(int argc, char* argv[]) {

	 Spike::Log::Init();
	 ENGINE_WARN("Initialized Log!");

     auto app = Spike::CreateApplication(argc, argv);
	 app->Tick();
	 delete app;

	 return 0;
}
#endif