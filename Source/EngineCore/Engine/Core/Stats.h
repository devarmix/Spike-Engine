#pragma once

#include <Engine/GUI/GUIWindow.h>
using namespace SpikeEngine;

namespace Spike {

	struct StatsData {

		float Fps;
		float Frametime;

		int TriangleCount;
		int DrawcallCount;
		float SceneUpdateTime;
		float RenderTime;
	};

	class StatsGUI : public GUIWindow {
	public:
		StatsGUI() : GUIWindow("Stats") {}
		virtual ~StatsGUI() override;

		virtual void OnCreate() override {}
		virtual void OnGUI(float deltaTime) override;
	};

	class Stats {
	public:
		static StatsData Data;
	};
}
