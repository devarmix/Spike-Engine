#include <Editor/Layers/EditorGUILayer.h>
#include <Editor/GUI/Scene/SceneViewport.h>
#include <Editor/Core/Editor.h>

namespace SpikeEditor {

	EditorGUILayer::~EditorGUILayer() {}

	void EditorGUILayer::OnAttach() {

		SpikeEditor::Get().CreateGUIWindow<SceneViewport>();
	}

	void EditorGUILayer::OnDetach() {}

	void EditorGUILayer::OnUpdate(float deltaTime) {}

	void EditorGUILayer::OnEvent(Spike::Event& event) {}
}