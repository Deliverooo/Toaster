#pragma once

#include "toast_os.hpp"

namespace toaster::os
{
	using byte  = BYTE;
	using word  = WORD;
	using dword = DWORD;
	using qword = unsigned __int64;

	using uint  = UINT;
	using ulong = ULONG;

	using wchar = WCHAR;
	using str   = LPSTR;
	using cstr  = LPCSTR;
	using wstr  = LPWSTR;
	using wcstr = LPCWSTR;

	using uintptr  = UINT_PTR;
	using intptr   = INT_PTR;
	using longptr  = LONG_PTR;
	using ulongptr = ULONG_PTR;

	using wparam  = uintptr;
	using lparam  = longptr;
	using lresult = longptr;

	using Handle   = HANDLE;
	using Wnd      = HWND;
	using Module   = HMODULE;
	using Instance = HINSTANCE;
}