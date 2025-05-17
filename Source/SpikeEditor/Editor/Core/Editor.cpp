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
		.WinInfo = winInfo
	};

	return new SpikeEditor::SpikeEditor(appInfo);
}



namespace SpikeEditor {

	SpikeEditor* SpikeEditor::m_Instance;

	SpikeEditor::SpikeEditor(const Spike::ApplicationCreateInfo& info) : Application(info) {

		assert(m_Instance == nullptr);
		m_Instance = this;

		m_GUILayer = new EditorGUILayer();
		PushLayer(m_GUILayer);
	}

	SpikeEditor::~SpikeEditor() {}
}