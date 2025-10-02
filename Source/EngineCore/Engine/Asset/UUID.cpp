#include "Engine/Asset/UUID.h"
#include <random>

namespace Spike {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	UUID::UUID() : m_ID(0) {}
	UUID::UUID(uint64_t id)	: m_ID(id) {}

	UUID UUID::Generate() {

		UUID id(s_UniformDistribution(s_Engine));
		return id;
	}
}