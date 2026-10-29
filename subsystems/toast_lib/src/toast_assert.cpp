#include "toast_lib/toast_assert.hpp"
#include <Windows.h>

#include <stdio.h>

void printAssertFailed(const char *p_file, int p_line, const char *p_function, const char *p_expression)
{
	printf("Assertion failed in: [%s : %d] : %s -> expr: \"%s\"\n", p_file, p_line, p_function, p_expression);
	MessageBox(NULL, "Assertion failed: check console", "Assertion failed", MB_ICONERROR);
}

void printAssertFailedMsg(const char *p_file, int p_line, const char *p_function, const char *p_expression, const char *p_message)
{
	printf("%s\n", p_message);
	printf("Assertion failed in: [%s : %d] : %s -> expr: \"%s\"\n", p_file, p_line, p_function, p_expression);
	MessageBox(NULL, "Assertion failed: check console", "Assertion failed", MB_ICONERROR);
}
