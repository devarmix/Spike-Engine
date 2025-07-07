#pragma once

#include <memory>

namespace Spike {

	class AssetID
	{
	public:
		AssetID();
		AssetID(uint64_t id);
		AssetID(const AssetID&) = default;

		static AssetID Generate();

		operator uint64_t() const { return m_ID; }
	private:
		uint64_t m_ID;
	};

}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Spike::AssetID>
	{
		std::size_t operator()(const Spike::AssetID& id) const
		{
			return (uint64_t)id;
		}
	};

}