#pragma once

#include "system_types.h"

namespace toaster
{
	// Better because it is 64 bit and more like a pointer
	template<typename Type>
	struct Handle
	{
		uint64 handle{0u};

		constexpr Handle() = default;

		constexpr Handle(uint32 p_id, uint32 p_magic) : handle(static_cast<uint64>(p_id) | (static_cast<uint64>(p_magic) << 32ui64))
		{
		}

		constexpr Handle(nulltype) : handle(0u) // This makes it look better
		{
		}

		explicit constexpr Handle(uint64 p_handle) : handle(p_handle)
		{
		}

		[[nodiscard]] constexpr auto getId() const -> uint32 { return static_cast<uint32>(handle & 0xFFFFFFFFui64); }
		[[nodiscard]] constexpr auto getMagic() const -> uint32 { return static_cast<uint32>((handle >> 32ui64) & 0xFFFFFFFFui64); }

		[[nodiscard]] constexpr auto valid() const -> bool { return getMagic() != 0ui32; } // Only checks if the handle itself is valid, not that the pool thinks it is

		auto operator<=>(const Handle &) const = default; // I need to be able to run algorithms on ts

		constexpr          operator bool() const { return valid(); }
		explicit constexpr operator uint64() const { return handle; }
	};
}

#define TST_DECLARE_HANDLE(__tag) \
using __tag##Handle = ::toaster::Handle<__tag>

template<typename Tag>
struct std::hash<toaster::Handle<Tag> >
{
	auto operator()(const toaster::Handle<Tag> &p_handle) const noexcept -> std::size_t
	{
		return hash<uint64>{}(p_handle.handle);
	}
};
