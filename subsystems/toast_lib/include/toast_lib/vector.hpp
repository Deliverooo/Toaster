#pragma once

#include <vector>

#include "toast_lib.hpp"

namespace toaster
{
	template<typename Type>
	using Vector = std::vector<Type, mi_stl_allocator<Type>>;
}