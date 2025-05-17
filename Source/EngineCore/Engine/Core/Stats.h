#pragma once

#include <Engine/GUI/GUIWindow.h>
using namespace SpikeEngine;

namespace Spike {

	struct StatsData {

		float fps;
		float frametime;

		int triangle_count;
		int drawcall_count;
		float scene_update_time;
		float mesh_draw_time;
	};

	class StatsGUI : public GUI_Window {
	public:
		StatsGUI() : GUI_Window("Stats") {}
		~StatsGUI() override;

		virtual void OnCreate() override {}
		virtual void OnGUI(float deltaTime) override;
	};

	class Stats {
	public:
		static StatsData stats;
	};
}
