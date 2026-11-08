#pragma once

#include <climits>
#include "array.hpp"

#define TST_BIT(__n) (1u << __n)

namespace toaster
{
	template<uint64 Size>
	struct Bitset
	{
		static constexpr uint64 c_bitSize{CHAR_BIT};

		static constexpr auto _bitset_size(const uint64 p_size) -> uint64 { return (p_size + c_bitSize - 1ull) / c_bitSize; }

		constexpr auto test(uint64 p_index) const -> bool { return _bits[p_index / c_bitSize] & TST_BIT(p_index % c_bitSize); }
		constexpr auto set(uint64 p_index) -> void { _bits[p_index / c_bitSize] |= TST_BIT(p_index % c_bitSize); }
		constexpr auto clear(uint64 p_index) -> void { _bits[p_index / c_bitSize] &= ~TST_BIT(p_index % c_bitSize); }
		constexpr auto reset() -> void { _bits.fill(0u); }

		constexpr auto operator[](uint64 p_index) const -> bool { return test(p_index); }

		Array<uint8, _bitset_size(Size)> _bits;
	};
}
