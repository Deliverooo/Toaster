#pragma once

#include "toast_assert.hpp"
#include "toast_lib.hpp"

namespace toaster
{
	template<typename Type>
	class Allocator
	{
	public:
		using value_type = Type;

		Allocator() noexcept = default;

		template<typename TOther>
		Allocator(const Allocator<TOther> &) noexcept
		{
		}

		[[nodiscard]] auto allocate(size_t p_n) -> Type *
		{
			if (p_n == 0u)
				return nullptr;

			void *ptr{rpmalloc(p_n * sizeof(Type))};
			if (!ptr)
			{
				TST_PERMA_ASSERT_MSG(false, "Failed to allocate pointer");
				return nullptr;
			}

			return static_cast<Type *>(ptr);
		}

		auto deallocate(Type *p_ptr, size_t) noexcept -> void
		{
			rpfree(p_ptr);
		}
	};

	template<typename Type, typename TOther>
	auto operator==(const Allocator<Type> &, const Allocator<TOther> &) noexcept -> bool
	{
		return true;
	}

	template<typename Type, typename TOther>
	auto operator!=(const Allocator<Type> &, const Allocator<TOther> &) noexcept -> bool
	{
		return false;
	}
}
