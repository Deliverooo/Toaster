#pragma once

#include <climits>
#include "system_types.h"

#define TST_BIT(__n) (1u << __n)

#define TST_BIT_CHAR_SIZE CHAR_BIT
#define TST_BITSET_SIZE(__size) (((__size) + TST_BIT_CHAR_SIZE - 1u) / TST_BIT_CHAR_SIZE)

#define TST_BIT_SET(__bitset, __index) ((__bitset)[(__index) / TST_BIT_CHAR_SIZE]) |= TST_BIT((__index % TST_BIT_CHAR_SIZE))
#define TST_BIT_CLEAR(__bitset, __index) ((__bitset)[(__index) / TST_BIT_CHAR_SIZE]) &= ~TST_BIT((__index % TST_BIT_CHAR_SIZE))
#define TST_BIT_TEST(__bitset, __index) (((__bitset)[(__index) / TST_BIT_CHAR_SIZE]) & TST_BIT(__index % TST_BIT_CHAR_SIZE))

#define TST_BITSET_RESET(__bitset, __size) for (uint32 i = 0u; i < TST_BITSET_SIZE(__size); ++i)\
													__bitset[i] = 0u

#define TST_BITSET(__name, __size) uint8 __name[TST_BITSET_SIZE(__size)]
