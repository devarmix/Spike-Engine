#pragma once

#include <memory>

namespace Spike {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t id);
		UUID(const UUID&) = default;

		static UUID Generate();
		operator uint64_t() const { return m_ID; }

		struct Hasher {
			size_t operator()(const Spike::UUID& id) const {
				return (uint64_t)id;
			}
		};

	private:
		uint64_t m_ID;
	};
}