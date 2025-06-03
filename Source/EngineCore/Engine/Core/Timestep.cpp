#include <Engine/Core/Timestep.h>

namespace Spike {

	void Timer::Reset() {

		m_Start = std::chrono::high_resolution_clock::now();
	}

	float Timer::GetElapsedMs() {

		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_Start).count() * 0.001f * 0.001f;
	}
}