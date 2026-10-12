#include "toast_lib/memory.hpp"

namespace toaster
{
	auto alignedAlloc(uint64 p_size, uint64 p_alignment) -> void *
	{
		return _aligned_malloc(p_size, p_alignment);
	}

	auto alignedFree(void *p_memory) -> void
	{
		_aligned_free(p_memory);
	}
}
