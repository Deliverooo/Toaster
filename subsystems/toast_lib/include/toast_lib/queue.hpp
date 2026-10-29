#pragma once

#include <queue>

#include "toast_lib.hpp"

namespace toaster
{
	template<typename Type>
	using Deque = std::deque<Type, mi_stl_allocator<Type>>;

	template<typename Type>
	using Queue = std::queue<Type, Deque<Type>>;
}