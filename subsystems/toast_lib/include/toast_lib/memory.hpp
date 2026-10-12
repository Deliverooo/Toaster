#pragma once

#include "system_types.h"

#include <memory>

#include "toast_lib.hpp"

namespace toaster
{
	constexpr uint64 maxAlignment{alignof(std::max_align_t)};

	[[nodiscard]] auto TST_LIB_API alignedAlloc(uint64 p_size, uint64 p_alignment) -> void *;
	auto TST_LIB_API               alignedFree(void *p_memory) -> void;
}
