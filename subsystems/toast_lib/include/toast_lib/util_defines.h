#pragma once

#define TST_ALIGN(__val, __alignment)  ((__val + (__alignment - 1ull)) & ~(__alignment - 1ull))
#define TST_ALIGN_PTR(__ptr, __alignment) (typeof(__ptr))( (void*) (((uintptr)__ptr + (__alignment - 1ull)) & ~(__alignment - 1ull)))

#define TST_UNUSED [[maybe_unused]]
