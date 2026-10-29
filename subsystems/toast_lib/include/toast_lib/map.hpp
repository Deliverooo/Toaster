#pragma once

#include <unordered_map>

#include "allocator.hpp"

namespace toaster
{
	template<typename TKey, typename TValue>
	using UnorderedMap = std::unordered_map<TKey, TValue, std::hash<TKey>, std::equal_to<>, Allocator<std::pair<const TKey, TValue> > >;
}
