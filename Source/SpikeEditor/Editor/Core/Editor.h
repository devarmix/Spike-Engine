#pragma once

#include <Engine/Spike.h>
#include <Editor/Layers/EditorGUILayer.h>

namespace SpikeEditor {

	class SpikeEditor : public Spike::Application {
	public:
		SpikeEditor(const Spike::ApplicationCreateInfo& info);
		~SpikeEditor() override;

		static SpikeEditor& Get() { return *m_Instance; }

	private:
		static SpikeEditor* m_Instance;

		EditorGUILayer* m_GUILayer;
	};
}
