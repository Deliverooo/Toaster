#pragma once

#include "system_types.hpp"

#define TST_OS_ANSI_WINMAIN() int WINAPI WinMain(::toaster::os::Instance p_instance, ::toaster::os::Instance p_prev_instance, ::toaster::os::str p_cmd_line, int p_show_cmd)
#define TST_OS_UNICODE_WINMAIN() int WINAPI wWinMain(::toaster::os::Instance p_instance, ::toaster::os::Instance p_prev_instance, ::toaster::os::wstr p_cmd_line, int p_show_cmd)

#define TST_OS_WINMAIN() TST_OS_UNICODE_WINMAIN()
