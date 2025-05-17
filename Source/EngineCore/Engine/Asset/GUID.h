#pragma once

#include <memory>

namespace Spike {

	class GUID
	{
	public:
		GUID();
		GUID(uint64_t guid);
		GUID(const GUID&) = default;

		static GUID Generate();

		operator uint64_t() const { return m_GUID; }
	private:
		uint64_t m_GUID;
	};

}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Spike::GUID>
	{
		std::size_t operator()(const Spike::GUID& guid) const
		{
			return (uint64_t)guid;
		}
	};

}