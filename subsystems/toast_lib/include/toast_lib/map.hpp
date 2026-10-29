#pragma once

#include <unordered_map>
#include "toast_lib.hpp"

namespace toaster
{
	template<typename TKey, typename TValue>
	using UnorderedMap = std::unordered_map<TKey, TValue, std::hash<TKey>, std::equal_to<>, mi_stl_allocator<std::pair<const TKey, TValue> > >;
}
