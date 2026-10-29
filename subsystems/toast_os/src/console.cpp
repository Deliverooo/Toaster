#include "toast_os/console.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>

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

			std::cout.clear();
			std::clog.clear();
			std::cerr.clear();
			std::cin.clear();

			std::ios_base::sync_with_stdio();

			printf("Console initialised!\n"); // If I don't print something, there is a chance that subsequent calls to printf will do nothing...
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
