#pragma once

#include <Engine/Spike.h>
#include <Editor/Layers/EditorLayer.h>

namespace SpikeEditor {

	class SpikeEditor : public Spike::Application {
	public:
		SpikeEditor(const Spike::ApplicationDesc& desc);
		virtual ~SpikeEditor() override;

	private:

		EditorLayer* m_EditorLayer;
	};

	// global editor pointer
	extern SpikeEditor* GEditor;
}
