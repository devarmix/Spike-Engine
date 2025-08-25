#include <Editor/Core/Editor.h>

// main file
#include <Engine/Core/Main.h>
#include <Engine/SpikeEngine.h>


// --Entry Point--
Spike::Application* Spike::CreateApplication() {

	WindowDesc winDesc{

		.Name = "Spike Editor",
		.Width = 1920,
		.Height = 1080
	};

	ApplicationDesc appDesc{

		.Name = "Spike Editor",
		.UsingImGui = true,
		.WindowDesc = winDesc
	};

	return new SpikeEditor::SpikeEditor(appDesc);
}

SpikeEditor::SpikeEditor* SpikeEditor::GEditor = nullptr;

namespace SpikeEditor {

	SpikeEditor::SpikeEditor(const Spike::ApplicationDesc& desc) : Application(desc) {

		GEditor = this;

		m_EditorLayer = new EditorLayer();
		PushLayer(m_EditorLayer);
	}

	SpikeEditor::~SpikeEditor() { Destroy(); }
}