#pragma once

#include "toast_assert.hpp"
#include "toast_lib.hpp"

namespace toaster
{
	using CString = const char *;

	using AllocationFailedCB = void(CString);

	struct Allocator
	{
		rpmalloc_interface_t mallocInterface;
	};

	inline auto allocator_create(Allocator &p_out_allocator, AllocationFailedCB p_allocation_failed_cb = nullptr) -> void
	{
		p_out_allocator.mallocInterface.error_callback = p_allocation_failed_cb;
		rpmalloc_initialize(&p_out_allocator.mallocInterface);
	}

	inline auto allocator_destroy([[maybe_unused]] Allocator &p_allocator) -> void
	{
		rpmalloc_finalize();
	}

	template<typename Type>
	struct STLAllocator
	{
		using value_type = Type;

		STLAllocator() noexcept = default;

		template<typename TOther>
		STLAllocator(const STLAllocator<TOther> &) noexcept
		{
		}

		[[nodiscard]] auto allocate(size_t p_n) -> Type *
		{
			if (p_n == 0u)
				return nullptr;

			void *ptr{rpmalloc(p_n * sizeof(Type))};
			if (!ptr)
			{
				TST_PERMA_ASSERT_MSG(false, "Failed to allocate pointer using standard library allocator");
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
	auto operator==(const STLAllocator<Type> &, const STLAllocator<TOther> &) noexcept -> bool
	{
		return true;
	}

	template<typename Type, typename TOther>
	auto operator!=(const STLAllocator<Type> &, const STLAllocator<TOther> &) noexcept -> bool
	{
		return false;
	}
}
