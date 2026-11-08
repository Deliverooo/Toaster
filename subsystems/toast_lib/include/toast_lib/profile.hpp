#pragma once

#ifdef TST_PROFILE_ENABLE
#include <tracy/Tracy.hpp>

#define TST_PROFILE_SCP_N(__name) ZoneScopedN(__name)

#else

#define TST_PROFILE_SCP_N(__name)

#endif
