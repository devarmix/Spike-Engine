#include "Engine/Asset/GUID.h"

#include <random>

#include <unordered_map>

namespace Spike {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	AssetID::AssetID()
		: m_ID(0)
	{
	}

	AssetID::AssetID(uint64_t id)
		: m_ID(id)
	{
	}

	AssetID AssetID::Generate() {

		AssetID id(s_UniformDistribution(s_Engine));
		return id;
	}
}