#pragma once

#include <chrono>

namespace Spike {

	class Time {
	public:

		float DeltaTime = 0;
	};

	class Timer {
	public:
		Timer() { Reset(); }
		~Timer() {}

		void Reset();
		float GetElapsedMs();

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};
}
