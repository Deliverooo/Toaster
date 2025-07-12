#pragma once

// this was pretty much entirely taken from glfw3.h
#define TST_KEY_SPACE              32
#define TST_KEY_APOSTROPHE         39  /* ' */
#define TST_KEY_COMMA              44  /* , */
#define TST_KEY_MINUS              45  /* - */
#define TST_KEY_PERIOD             46  /* . */
#define TST_KEY_SLASH              47  /* / */
#define TST_KEY_0                  48
#define TST_KEY_1                  49
#define TST_KEY_2                  50
#define TST_KEY_3                  51
#define TST_KEY_4                  52
#define TST_KEY_5                  53
#define TST_KEY_6                  54
#define TST_KEY_7                  55
#define TST_KEY_8                  56
#define TST_KEY_9                  57
#define TST_KEY_SEMICOLON          59  /* ; */
#define TST_KEY_EQUAL              61  /* = */
#define TST_KEY_A                  65
#define TST_KEY_B                  66
#define TST_KEY_C                  67
#define TST_KEY_D                  68
#define TST_KEY_E                  69
#define TST_KEY_F                  70
#define TST_KEY_G                  71
#define TST_KEY_H                  72
#define TST_KEY_I                  73
#define TST_KEY_J                  74
#define TST_KEY_K                  75
#define TST_KEY_L                  76
#define TST_KEY_M                  77
#define TST_KEY_N                  78
#define TST_KEY_O                  79
#define TST_KEY_P                  80
#define TST_KEY_Q                  81
#define TST_KEY_R                  82
#define TST_KEY_S                  83
#define TST_KEY_T                  84
#define TST_KEY_U                  85
#define TST_KEY_V                  86
#define TST_KEY_W                  87
#define TST_KEY_X                  88
#define TST_KEY_Y                  89
#define TST_KEY_Z                  90
#define TST_KEY_LEFT_BRACKET       91  /* [ */
#define TST_KEY_BACKSLASH          92  /* \ */
#define TST_KEY_RIGHT_BRACKET      93  /* ] */
#define TST_KEY_GRAVE_ACCENT       96  /* ` */
#define TST_KEY_WORLD_1            161 /* non-US #1 */
#define TST_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define TST_KEY_ESCAPE             256
#define TST_KEY_ENTER              257
#define TST_KEY_TAB                258
#define TST_KEY_BACKSPACE          259
#define TST_KEY_INSERT             260
#define TST_KEY_DELETE             261
#define TST_KEY_RIGHT              262
#define TST_KEY_LEFT               263
#define TST_KEY_DOWN               264
#define TST_KEY_UP                 265
#define TST_KEY_PAGE_UP            266
#define TST_KEY_PAGE_DOWN          267
#define TST_KEY_HOME               268
#define TST_KEY_END                269
#define TST_KEY_CAPS_LOCK          280
#define TST_KEY_SCROLL_LOCK        281
#define TST_KEY_NUM_LOCK           282
#define TST_KEY_PRINT_SCREEN       283
#define TST_KEY_PAUSE              284
#define TST_KEY_F1                 290
#define TST_KEY_F2                 291
#define TST_KEY_F3                 292
#define TST_KEY_F4                 293
#define TST_KEY_F5                 294
#define TST_KEY_F6                 295
#define TST_KEY_F7                 296
#define TST_KEY_F8                 297
#define TST_KEY_F9                 298
#define TST_KEY_F10                299
#define TST_KEY_F11                300
#define TST_KEY_F12                301
#define TST_KEY_F13                302
#define TST_KEY_F14                303
#define TST_KEY_F15                304
#define TST_KEY_F16                305
#define TST_KEY_F17                306
#define TST_KEY_F18                307
#define TST_KEY_F19                308
#define TST_KEY_F20                309
#define TST_KEY_F21                310
#define TST_KEY_F22                311
#define TST_KEY_F23                312
#define TST_KEY_F24                313
#define TST_KEY_F25                314
#define TST_KEY_KP_0               320
#define TST_KEY_KP_1               321
#define TST_KEY_KP_2               322
#define TST_KEY_KP_3               323
#define TST_KEY_KP_4               324
#define TST_KEY_KP_5               325
#define TST_KEY_KP_6               326
#define TST_KEY_KP_7               327
#define TST_KEY_KP_8               328
#define TST_KEY_KP_9               329
#define TST_KEY_KP_DECIMAL         330
#define TST_KEY_KP_DIVIDE          331
#define TST_KEY_KP_MULTIPLY        332
#define TST_KEY_KP_SUBTRACT        333
#define TST_KEY_KP_ADD             334
#define TST_KEY_KP_ENTER           335
#define TST_KEY_KP_EQUAL           336
#define TST_KEY_LEFT_SHIFT         340
#define TST_KEY_LEFT_CONTROL       341
#define TST_KEY_LEFT_ALT           342
#define TST_KEY_LEFT_SUPER         343
#define TST_KEY_RIGHT_SHIFT        344
#define TST_KEY_RIGHT_CONTROL      345
#define TST_KEY_RIGHT_ALT          346
#define TST_KEY_RIGHT_SUPER        347
#define TST_KEY_MENU               348

//int TstKeyCodeToGlfwKeyCode(int keycode)
//{
//	switch (keycode)
//	{
//		case TST_KEY_SPACE:
//			return GLFW_KEY_SPACE;
//		case TST_KEGLFWPOSTROPHE:
//			return GLFW_KEY_APOSTROPHE;
//		case TST_KEGLFWOMMA:
//			return GLFW_KEY_COMMA;
//		case TST_KEGLFWINUS:
//			return GLFW_KEY_MINUS;
//		case TST_KEGLFWERIOD:
//			return GLFW_KEY_PERIOD;
//		case TST_KEGLFWLASH:
//			return GLFW_KEY_SLASH;
//		case TST_KEGLFW:
//			return GLFW_KEY_0;
//		case TST_KEGLFW:
//			return GLFW_KEY_1;
//		case TST_KEGLFW:
//			return GLFW_KEY_2;
//		case TST_KEGLFW:
//			return GLFW_KEY_3;
//		case TST_KEGLFW:
//			return GLFW_KEY_4;
//		case TST_KEGLFW:
//			return GLFW_KEY_5;
//		case TST_KEGLFW:
//			return GLFW_KEY_6;
//		case TST_KEGLFW:
//			return GLFW_KEY_7;
//		case TST_KEGLFW:
//			return GLFW_KEY_8;
//		case TST_KEGLFW:
//			return GLFW_KEY_9;
//		case TST_KEGLFWEMICOLON:
//			return GLFW_KEY_SEMICOLON;
//		case TST_KEGLFWQUAL:
//			return GLFW_KEY_EQUAL;
//		case TST_KEGLFW:
//			return GLFW_KEY_A;
//		case TST_KEGLFW:
//			return GLFW_KEY_B;
//		case TST_KEGLFW:
//			return GLFW_KEY_C;
//		case TST_KEGLFW:
//			return GLFW_KEY_D;
//		case TST_KEGLFW:
//			return GLFW_KEY_E;
//		case TST_KEGLFW:
//			return GLFW_KEY_F;
//		case TST_KEGLFW:
//			return GLFW_KEY_G;
//		case TST_KEGLFW:
//			return GLFW_KEY_H;
//		case TST_KEGLFW:
//			return GLFW_KEY_I;
//		case TST_KEGLFW:
//			return GLFW_KEY_J;
//		case TST_KEGLFW:
//			return GLFW_KEY_K;
//		case TST_KEGLFW:
//			return GLFW_KEY_L;
//		case TST_KEGLFW:
//			return GLFW_KEY_M;
//		case TST_KEGLFW:
//			return GLFW_KEY_N;
//		case TST_KEGLFW:
//			return GLFW_KEY_O;
//		case TST_KEGLFW:
//			return GLFW_KEY_P;
//		case TST_KEGLFW:
//			return GLFW_KEY_Q;
//		case TST_KEGLFW:
//			return GLFW_KEY_R;
//		case TST_KEGLFW:
//			return GLFW_KEY_S;
//		case TST_KEGLFW:
//			return GLFW_KEY_T;
//		case TST_KEGLFW:
//			return GLFW_KEY_U;
//		case TST_KEGLFW:
//			return GLFW_KEY_V;
//		case TST_KEGLFW:
//			return GLFW_KEY_W;
//		case TST_KEGLFW:
//			return GLFW_KEY_X;
//		case TST_KEGLFW:
//			return GLFW_KEY_Y;
//		case TST_KEGLFW:
//			return GLFW_KEY_Z;
//		case TST_KEGLFWEFT_BRACKET:
//			return GLFW_KEY_LEFT_BRACKET;
//		case TST_KEGLFWACKSLASH:
//			return GLFW_KEY_BACKSLASH;
//		case TST_KEGLFWIGHT_BRACKET:
//			return GLFW_KEY_RIGHT_BRACKET;
//		case TST_KEGLFWRAVE_ACCENT:
//			return GLFW_KEY_GRAVE_ACCENT;
//		case TST_KEGLFWORLD_1:
//			return GLFW_KEY_WORLD_1;
//		case TST_KEGLFWORLD_2:
//			return GLFW_KEY_WORLD_2;
//
//
//	}
//}
