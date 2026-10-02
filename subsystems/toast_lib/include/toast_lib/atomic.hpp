#pragma once

#include <atomic>

namespace toaster
{
	#define TST_ALIGN_ATOMIC(__type) alignas(std::atomic_ref<__type>::required_alignment) __type

	#define TST_SCOPED_ATOMIC(__var, __name) std::atomic_ref<decltype(__var)> __name{__var}

	// Scoped lock
	#define TST_SCP_LOCK(__mutex) std::scoped_lock<std::mutex> _lock_##mutex{__mutex};
}