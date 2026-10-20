#include "toast_os/console.hpp"

#include <cassert>
#include <cstdio>

namespace toaster::os
{
	void output_console_create(cstr p_title)
	{
		if (AllocConsole())
		{
			FILE *fp{nullptr};
			freopen_s(&fp, "CONOUT$", "w", stdout);
			freopen_s(&fp, "CONOUT$", "w", stderr);
			freopen_s(&fp, "CONIN$", "r", stdin);
		}
		else
		{
			assert(false && "Failed to create output console");
			abort();
		}
		SetConsoleTitle(p_title);
	}

	void output_console_destroy()
	{
		FreeConsole();
	}
}
