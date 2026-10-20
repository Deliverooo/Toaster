#pragma once

#include "system_types.hpp"
#include "toast_os.hpp"

namespace toaster::os
{
	// Creates a console and redirects all input/output to it
	void TST_OS_API output_console_create(cstr p_title);
	void TST_OS_API output_console_destroy();
}
