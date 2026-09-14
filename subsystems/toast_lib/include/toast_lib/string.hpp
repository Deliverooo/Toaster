#pragma once

#include "toast_lib.hpp"

#include <string>
#include <string_view>

namespace toaster
{
	using CString  = const char *;
	using CWString = const wchar_t *;

	using String     = std::string;
	using StringView = std::string_view;
	using WString    = std::wstring;
	using U8String   = std::u8string;
	using U16String  = std::u16string;
	using U32String  = std::u32string;

	template<typename Type> requires std::is_arithmetic_v<Type>
	auto to_string(Type p_val) -> String
	{
		return std::to_string(p_val);
	}

	template<typename Type> requires std::is_arithmetic_v<Type>
	auto to_wstring(Type p_val) -> WString
	{
		return std::to_wstring(p_val);
	}

	template<typename Type> requires std::is_arithmetic_v<Type>
	auto to_u8string(Type p_val) -> U8String
	{
		std::string ret = std::to_string(p_val);
		return U8String{ret.begin(), ret.end()};
	}

	template<typename Type> requires std::is_arithmetic_v<Type>
	auto to_u16string(Type p_val) -> U16String
	{
		std::string ret = std::to_string(p_val);
		return U16String{ret.begin(), ret.end()};
	}

	template<typename Type> requires std::is_arithmetic_v<Type>
	auto to_u32string(Type p_val) -> U32String
	{
		std::string ret = std::to_string(p_val);
		return U32String{ret.begin(), ret.end()};
	}

	TST_LIB_API WString convertUtf8ToWide(const String &p_string);
	TST_LIB_API String  convertWideToUtf8(const WString &p_string);
}
