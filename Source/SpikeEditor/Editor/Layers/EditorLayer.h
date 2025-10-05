#pragma once

#include <Engine/Core/Layer.h>
#include <Editor/Renderer/Widget.h>

namespace Spike {

	class EditorLayer : public Layer {
	public:
		EditorLayer();
		~EditorLayer() override;

		virtual void Tick(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(const GenericEvent& event) override;
	private:
		void ConfigImGui();

	private:
		WorldViewportWidget m_WorldViewport;
	};
}
