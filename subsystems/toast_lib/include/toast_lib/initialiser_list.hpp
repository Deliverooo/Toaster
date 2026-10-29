#pragma once

#include <initializer_list>
#include <type_traits>

#include "system_types.h"

namespace toaster
{
	template<typename Type>
	class InitialiserList
	{
	public:
		constexpr InitialiserList() noexcept : m_ptr(nullptr), m_count(0u)
		{
		}

		constexpr InitialiserList(nulltype) noexcept : m_ptr(nullptr), m_count(0u)
		{
		}

		constexpr InitialiserList(const Type &p_value) noexcept : m_ptr(&p_value), m_count(1u)
		{
		}

		constexpr InitialiserList(const Type *p_ptr, uint32 p_count) noexcept : m_ptr(p_ptr), m_count(p_count)
		{
		}

		template<uint64 Size>
		constexpr InitialiserList(const Type (&p_ptr)[Size]) noexcept : m_ptr(p_ptr), m_count(Size)
		{
		}

		constexpr InitialiserList(const std::initializer_list<Type> &p_initialiser_list) noexcept : m_ptr(p_initialiser_list.begin()), m_count(p_initialiser_list.size())
		{
		}

		template<typename TContainer> requires std::is_convertible_v<decltype(std::declval<TContainer>().data()), Type *> && std::is_convertible_v<decltype(std::declval<
												   TContainer>().size()), std::size_t>
		constexpr InitialiserList(const TContainer &p_container) : m_ptr(p_container.data()), m_count(p_container.size())
		{
		}

		auto begin() const noexcept -> const Type * { return m_ptr; }

		auto end() const noexcept -> const Type * { return m_ptr + m_count; }

		auto front() const noexcept -> const Type & { return *m_ptr; }
		auto back() const noexcept -> const Type & { return *(m_ptr + m_count - 1); }

		auto empty() const -> bool { return !m_count; }
		auto size() const -> uint32 { return m_count; }
		auto data() const -> const Type * { return m_ptr; }

		auto operator[](uint32 p_index) const -> const Type & { return m_ptr[p_index]; }

	private:
		Type * m_ptr{nullptr};
		uint32 m_count{0u};
	};
}