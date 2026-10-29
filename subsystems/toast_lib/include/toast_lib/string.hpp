#pragma once

#include "allocator.hpp"

#include <string_view>
#include <xstring>

namespace toaster
{
	using CString  = const char *;
	using CWString = const wchar_t *;

	using String     = std::basic_string<char, std::char_traits<char>, Allocator<char> >;
	using StringView = std::string_view;
	using WString    = std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator<wchar_t> >;
	using U8String   = std::basic_string<char8_t, std::char_traits<char8_t>, Allocator<char8_t> >;
	using U16String  = std::basic_string<char16_t, std::char_traits<char16_t>, Allocator<char16_t> >;
	using U32String  = std::basic_string<char32_t, std::char_traits<char32_t>, Allocator<char32_t> >;
}
