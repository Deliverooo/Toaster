#pragma once

#include <format>

#include "array.hpp"

namespace toaster
{
	struct TST_LIB_API ByteSize
	{
		uint64 size{0u};
		bool   base2Units{false};
	};
}

template<>
struct std::formatter<toaster::ByteSize>
{
	constexpr auto parse(format_parse_context &p_ctx)
	{
		return p_ctx.begin();
	}

	auto format(const toaster::ByteSize &p_byte_size, std::format_context &p_ctx) const
	{
		constexpr toaster::Array<std::string_view, 6u> decimal_units{"B", "KB", "MB", "GB", "TB", "PB"};
		constexpr toaster::Array<std::string_view, 6u> binary_units{"B", "KiB", "MiB", "GiB", "TiB", "PiB"};

		const float64 base{p_byte_size.base2Units ? 1024.0 : 1000.0};
		const auto &  units{p_byte_size.base2Units ? binary_units : decimal_units};

		float64 size{static_cast<float64>(p_byte_size.size)};
		uint64  unit_index{0u};

		while (size >= base && unit_index < units.size() - 1u)
		{
			size /= base;
			++unit_index;
		}

		if (unit_index == 0u)
			return std::format_to(p_ctx.out(), "{} {}", p_byte_size.size, units[unit_index]);

		return std::format_to(p_ctx.out(), "{:.2f} {}", size, units[unit_index]);
	}
};
