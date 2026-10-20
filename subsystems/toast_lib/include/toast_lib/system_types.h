#pragma once

#include <cstddef> // (C++) std::nullptr_t
#include <cstdint> // _t typedefs

using bool32 = uint32_t; // You could use this instead of bool inside of a vector... IFYKYK

using nulltype = nullptr_t;

using intptr  = intptr_t;
using uintptr = uintptr_t;

using int8  = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using float32 = float;
using float64 = double;

using char8  = char8_t;
using char16 = char16_t;
using char32 = char32_t;