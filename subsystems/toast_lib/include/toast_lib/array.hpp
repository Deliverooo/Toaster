#pragma once

#include "system_types.h"
#include "toast_assert.h"

namespace toaster
{
	template<typename Type, uint64 Size>
	struct TST_LIB_API Array
	{
		using Iterator      = Type *;
		using ConstIterator = const Type *;

		constexpr auto begin() -> Iterator { return &_data[0ull]; }
		constexpr auto begin() const -> ConstIterator { return &_data[0ull]; }
		constexpr auto end() -> Iterator { return &_data[0ull] + Size; }
		constexpr auto end() const -> ConstIterator { return &_data[0ull] + Size; }

		constexpr auto front() -> Type & { return _data[0ull]; }
		constexpr auto front() const -> const Type & { return _data[0ull]; }
		constexpr auto back() -> Type & { return _data[Size - 1ull]; }
		constexpr auto back() const -> const Type & { return _data[Size - 1ull]; }

		constexpr auto data() -> Type * { return _data; }
		constexpr auto data() const -> const Type * { return _data; }

		[[nodiscard]] constexpr auto size() const -> uint64 { return Size; }

		constexpr auto fill(const Type &p_element) -> void
		{
			for (uint64 i{0ull}; i < Size; ++i)
				_data[i] = p_element;
		}

		constexpr auto operator[](uint64 p_index) -> Type &
		{
			// if consteval
			// {
				// static_assert(p_index < Size && "Index out of bounds");
			// }
			return _data[p_index];
		}

		constexpr auto operator[](uint64 p_index) const -> const Type &
		{
			// if consteval
			// {
				// static_assert(p_index < Size && "Index out of bounds");
			// }
			return _data[p_index];
		}

		Type _data[Size];
	};
}
