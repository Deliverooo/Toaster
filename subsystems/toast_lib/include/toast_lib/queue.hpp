#pragma once

#include <queue>

#include "allocator.hpp"

namespace toaster
{
	template<typename Type>
	using Deque = std::deque<Type, Allocator<Type>>;

	template<typename Type>
	using Queue = std::queue<Type, Deque<Type>>;
}