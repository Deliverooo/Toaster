#pragma once

#include <vector>

#include "allocator.hpp"

namespace toaster
{
	template<typename Type>
	using Vector = std::vector<Type, Allocator<Type>>;
}