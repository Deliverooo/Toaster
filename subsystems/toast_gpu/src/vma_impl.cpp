#define VMA_IMPLEMENTATION
#define VMA_CALL_PRE __declspec(dllexport)
#include <vk_mem_alloc.h>
// I want to include vma in the allocator header, so I can't define the vma implementation there :(
