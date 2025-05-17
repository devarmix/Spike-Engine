#include "Engine/Asset/GUID.h"

#include <random>

#include <unordered_map>

namespace Spike {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	GUID::GUID()
		: m_GUID(0)
	{
	}

	GUID::GUID(uint64_t guid)
		: m_GUID(guid)
	{
	}

	GUID GUID::Generate() {

		GUID guid(s_UniformDistribution(s_Engine));
		return guid;
	}
}