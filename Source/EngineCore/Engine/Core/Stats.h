#pragma once

namespace Spike {

	struct StatsData {

		float Fps;
		float Frametime;

		int TriangleCount;
		int DrawcallCount;
		float SceneUpdateTime;
		float RenderTime;
	};

	class Stats {
	public:
		static StatsData Data;
	};
}
