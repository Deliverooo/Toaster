#pragma once

#include "toast_lib.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

TST_LIB_API void printAssertFailed(const char *p_file, int p_line, const char *p_function, const char *p_expression);
TST_LIB_API void printAssertFailedMsg(const char *p_file, int p_line, const char *p_function, const char *p_expression, const char *p_message);

#ifndef NDEBUG

#define TST_ASSERT(__expr)\
	do { if(!(__expr))\
	{\
	printAssertFailed(__FILE__, __LINE__, __func__, #__expr);\
	__debugbreak();\
	} } while(false)

#define TST_ASSERT_MSG(__expr, __msg)\
	do { if(!(__expr))\
	{\
	printAssertFailedMsg(__FILE__, __LINE__, __func__, #__expr, __msg);\
	__debugbreak();\
	} } while(false)

#else

#define TST_ASSERT(_expr)
#define TST_ASSERT_MSG(_expr, _msg)

#endif

#define TST_PERMA_ASSERT(__expr)\
		do { if(!(__expr))\
		{\
		printAssertFailed(__FILE__, __LINE__, __func__, #__expr);\
		abort();\
		} } while(false)
#define TST_PERMA_ASSERT_MSG(__expr, __msg)\
		do { if(!(__expr))\
		{\
		printAssertFailedMsg(__FILE__, __LINE__, __func__, #__expr, __msg);\
		abort();\
		} } while(false)

#ifdef __cplusplus
}
#endif
