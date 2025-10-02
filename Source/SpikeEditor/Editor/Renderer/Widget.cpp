#include <Editor/Renderer/Widget.h>
#include <imgui/imgui.h>

namespace Spike {

	void WorldViewportWidget::Tick(float deltaTime) {

		ImGui::Begin("World");
		ImGui::Text("Test window for now, later a world viewport!");
		ImGui::End();
	}
}