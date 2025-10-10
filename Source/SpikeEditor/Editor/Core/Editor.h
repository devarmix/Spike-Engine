#pragma once

#include <Engine/Spike.h>
#include <Editor/Layers/EditorLayer.h>

namespace Spike {

	class SpikeEditor : public Spike::Application {
	public:
		SpikeEditor(const Spike::ApplicationDesc& desc, const std::filesystem::path& projectPath);
		virtual ~SpikeEditor() override;

		const std::filesystem::path& GetProjectPath() const { return m_ProjectPath; }
		static SpikeEditor& Get() { assert(s_Instance); return *s_Instance; }

	private:
		EditorLayer* m_EditorLayer;
		std::filesystem::path m_ProjectPath;

		static SpikeEditor* s_Instance;
	};
}
