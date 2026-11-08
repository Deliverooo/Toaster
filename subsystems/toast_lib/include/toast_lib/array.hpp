#pragma once

#include "system_types.h"
#include "toast_lib.hpp"

#include <initializer_list>
#include <memory>

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
		[[nodiscard]] constexpr auto size_bytes() const -> uint64 { return Size * sizeof(Type); }

		constexpr auto fill(const Type &p_element) -> void
		{
			for (uint64 i{0ull}; i < Size; ++i)
				_data[i] = p_element;
		}

		template<typename TFunc> requires std::is_invocable_r_v<void, TFunc, Type &, uint64> // Type and array index
		constexpr auto for_each(TFunc &&p_func) -> void
		{
			for (uint64 i{0ull}; i < Size; ++i)
				p_func(_data[i], i);
		}

		template<typename TFunc> requires std::is_invocable_r_v<void, TFunc, const Type &, uint64> // Type and array index
		constexpr auto for_each(TFunc &&p_func) const -> void
		{
			for (uint64 i{0ull}; i < Size; ++i)
				p_func(_data[i], i);
		}

		constexpr auto operator[](uint64 p_index) -> Type & { return _data[p_index]; }
		constexpr auto operator[](uint64 p_index) const -> const Type & { return _data[p_index]; }

		Type _data[Size];
	};

	// I got this from std::array
	template<typename TFirst, typename... TRest> requires std::is_same_v<TFirst, TRest...>
	Array(TFirst, TRest...) -> Array<std::decay_t<TFirst>, 1u + sizeof...(TRest)>;

	// Allows you to use enum classes to index into arrays. This is very useful...
	template<typename Type, typename TEnumClass, TEnumClass MaxElement> requires std::is_enum_v<TEnumClass>
	struct TST_LIB_API EArray
	{
		using Iterator      = Type *;
		using ConstIterator = const Type *;

		static constexpr uint64 c_size{static_cast<uint64>(MaxElement)};

		constexpr auto begin() -> Iterator { return &_data[0ull]; }
		constexpr auto begin() const -> ConstIterator { return &_data[0ull]; }
		constexpr auto end() -> Iterator { return &_data[0ull] + c_size; }
		constexpr auto end() const -> ConstIterator { return &_data[0ull] + c_size; }

		constexpr auto front() -> Type & { return _data[0ull]; }
		constexpr auto front() const -> const Type & { return _data[0ull]; }
		constexpr auto back() -> Type & { return _data[c_size - 1ull]; }
		constexpr auto back() const -> const Type & { return _data[c_size - 1ull]; }

		constexpr auto data() -> Type * { return _data; }
		constexpr auto data() const -> const Type * { return _data; }

		[[nodiscard]] constexpr auto size() const -> uint64 { return c_size; }
		[[nodiscard]] constexpr auto size_bytes() const -> uint64 { return c_size * sizeof(Type); }

		constexpr auto fill(const Type &p_element) -> void
		{
			for (uint64 i{0ull}; i < c_size; ++i)
				_data[i] = p_element;
		}

		template<typename TFunc> requires std::is_invocable_r_v<void, TFunc, Type &, uint64> // Type and array index
		constexpr auto for_each(TFunc &&p_func) -> void
		{
			for (uint64 i{0ull}; i < c_size; ++i)
				p_func(_data[i], i);
		}

		template<typename TFunc> requires std::is_invocable_r_v<void, TFunc, const Type &, uint64> // Type and array index
		constexpr auto for_each(TFunc &&p_func) const -> void
		{
			for (uint64 i{0ull}; i < c_size; ++i)
				p_func(_data[i], i);
		}

		constexpr auto operator[](TEnumClass p_index) -> Type & { return _data[static_cast<uint64>(p_index)]; }
		constexpr auto operator[](TEnumClass p_index) const -> const Type & { return _data[static_cast<uint64>(p_index)]; }

		constexpr auto operator[](uint64 p_index) -> Type & { return _data[p_index]; }
		constexpr auto operator[](uint64 p_index) const -> const Type & { return _data[p_index]; }

		Type _data[c_size];
	};
}
