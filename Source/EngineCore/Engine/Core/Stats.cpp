#include <Engine/Core/Stats.h>

namespace Spike {

	StatsData Stats::Data{};

	StatsGUI::~StatsGUI() {}

	void StatsGUI::OnGUI(float deltaTime) {

		ImGui::Text("Fps %f", Stats::Data.Fps);
		ImGui::Text("Frametime %f ms", Stats::Data.Frametime);
		ImGui::Text("Draw Time %f ms", Stats::Data.MeshDrawTime);
		ImGui::Text("Update Time %f ms", Stats::Data.SceneUpdateTime);
		ImGui::Text("Triangles %i", Stats::Data.TriangleCount);
		ImGui::Text("Draws %i", Stats::Data.DrawcallCount);
	}
}