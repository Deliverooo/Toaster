#pragma once

#include "system_types.h"
#include "toast_lib.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TstLib_Vector
{
	void * data{nullptr};
	uint64 size{0ull};
	uint64 capacity{0ull};
	uint64 elementSize{0ull};
} TstLib_Vector;

void TST_LIB_API tst_lib_vector_create(TstLib_Vector *p_out_vector, sizetype p_element_size);
void TST_LIB_API tst_lib_vector_destroy(TstLib_Vector *p_vector);

void TST_LIB_API tst_lib_vector_push_back(TstLib_Vector *p_vector, const void *p_element);

void TST_LIB_API tst_lib_vector_get();

#ifdef __cplusplus
}
#endif
