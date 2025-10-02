#include <Editor/Core/Editor.h>

// main file
#include <Engine/Core/Main.h>
#include <Engine/SpikeEngine.h>


// --Entry Point--
Spike::Application* Spike::CreateApplication(int argc, char* argv[]) {

	WindowDesc winDesc{

		.Name = "Spike Editor",
		.Width = 1920,
		.Height = 1080
	};

	ApplicationDesc appDesc{

		.Name = "Spike Editor",
		.UsingImGui = true,
		.UsingDocking = true,
		.WindowDesc = winDesc
	};

	//assert(argc > 1 && "No project path was specified!");
	return new Spike::SpikeEditor(appDesc, "C:/Users/Artem/Documents/Spike-Projects/Test");
}

Spike::SpikeEditor* Spike::GEditor = nullptr;

namespace Spike {

	SpikeEditor::SpikeEditor(const Spike::ApplicationDesc& desc, const std::filesystem::path& projectPath) : Application(desc), m_ProjectPath(projectPath) {
		GEditor = this;

		m_EditorLayer = new EditorLayer();
		PushLayer(m_EditorLayer);
	}

	SpikeEditor::~SpikeEditor() { Destroy(); }
}