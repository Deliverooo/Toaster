#pragma once

#include <unordered_set>

#include "allocator.hpp"

namespace toaster
{
	template<typename Type>
	using UnorderedSet = std::unordered_set<Type, std::hash<Type>, std::equal_to<>, Allocator<Type> >;
}
