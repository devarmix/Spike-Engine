#pragma once

#include <Engine/Spike.h>
#include <Editor/Renderer/EditorCamera.h>

namespace SpikeEditor {

	class EditorGUILayer : public Spike::Layer {
	public:
		EditorGUILayer() : Layer("Editor GUI Layer") {}
		~EditorGUILayer() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;

		virtual void OnEvent(Spike::Event& event) override;
	};
}