#include <Engine/Core/Stats.h>

namespace Spike {

	StatsData Stats::stats{};

	StatsGUI::~StatsGUI() {}

	void StatsGUI::OnGUI(float deltaTime) {

		ImGui::Text("fps %f", Stats::stats.fps);
		ImGui::Text("frametime %f ms", Stats::stats.frametime);
		ImGui::Text("draw time %f ms", Stats::stats.mesh_draw_time);
		ImGui::Text("update time %f ms", Stats::stats.scene_update_time);
		ImGui::Text("triangles %i", Stats::stats.triangle_count);
		ImGui::Text("draws %i", Stats::stats.drawcall_count);
	}
}