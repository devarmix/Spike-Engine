#include <Editor/Core/Editor.h>

// main file
#include <Engine/Core/Main.h>

#include <Engine/SpikeEngine.h>
using namespace SpikeEngine;


// --Entry Point--
Spike::Application* Spike::CreateApplication() {

	WindowCreateInfo winInfo{

		.Name = "Spike Editor",
		.Width = 1920,
		.Height = 1080
	};

	ApplicationCreateInfo appInfo{

		.Name = "Spike Editor",
		.UsingImGui = true,
		.WinInfo = winInfo
	};

	return new SpikeEditor::SpikeEditor(appInfo);
}

SpikeEditor::SpikeEditor* SpikeEditor::GEditor = nullptr;

namespace SpikeEditor {

	SpikeEditor::SpikeEditor(const Spike::ApplicationCreateInfo& info) : Application(info) {

		GEditor = this;

		m_EditorLayer = new EditorLayer();
		PushOverlay(m_EditorLayer);
	}

	SpikeEditor::~SpikeEditor() { Destroy(); }
}