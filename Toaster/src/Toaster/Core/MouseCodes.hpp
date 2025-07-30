#pragma once

// this was pretty much entirely taken from glfw3.h

enum class TstMouseCode
{
	ButtonLeft = 0,
	ButtonRight = 1,
	ButtonMiddle = 2,
	Button4 = 4,
	Button5 = 5,
	Button6 = 6,
	Button7 = 7,
	Button8 = 8,
};


#define TST_MOUSE_BUTTON_1         0
#define TST_MOUSE_BUTTON_2         1
#define TST_MOUSE_BUTTON_3         2
#define TST_MOUSE_BUTTON_4         3
#define TST_MOUSE_BUTTON_5         4
#define TST_MOUSE_BUTTON_6         5
#define TST_MOUSE_BUTTON_7         6
#define TST_MOUSE_BUTTON_8         7
#define TST_MOUSE_BUTTON_LAST      TST_MOUSE_BUTTON_8
#define TST_MOUSE_BUTTON_LEFT      TST_MOUSE_BUTTON_1
#define TST_MOUSE_BUTTON_RIGHT     TST_MOUSE_BUTTON_2
#define TST_MOUSE_BUTTON_MIDDLE    TST_MOUSE_BUTTON_3