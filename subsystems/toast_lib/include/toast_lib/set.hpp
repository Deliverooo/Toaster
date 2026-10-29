#pragma once

#include <unordered_set>

#include "toast_lib.hpp"

namespace toaster
{
	template<typename Type>
	using UnorderedSet = std::unordered_set<Type, std::hash<Type>, std::equal_to<>, mi_stl_allocator<Type> >;
}
