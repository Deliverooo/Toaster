#pragma once

namespace tst
{
	using KeyCode = uint16_t;

	namespace Key
	{

		enum : KeyCode
		{
			Space = 32,
			Apostrophe = 39,  /* ' */
			Comma = 44,  /* , */
			Minus = 45,  /* - */
			Period = 46,  /* . */
			Slash = 47,  /* / */
			Num0 = 48,
			Num1 = 49,
			Num2 = 50,
			Num3 = 51,
			Num4 = 52,
			Num5 = 53,
			Num6 = 54,
			Num7 = 55,
			Num8 = 56,
			Num9 = 57,
			Semicolon = 59,  /* ; */
			Equal = 61,  /* = */
			A = 65,
			B = 66,
			C = 67,
			D = 68,
			E = 69,
			F = 70,
			G = 71,
			H = 72,
			I = 73,
			J = 74,
			K = 75,
			L = 76,
			M = 77,
			N = 78,
			O = 79,
			P = 80,
			Q = 81,
			R = 82,
			S = 83,
			T = 84,
			U = 85,
			V = 86,
			W = 87,
			X = 88,
			Y = 89,
			Z = 90,
			LeftBracket = 91,  /* [ */
			Backslash = 92,  /* \ */
			RightBracket = 93,  /* ] */
			GraveAccent = 96,  /* ` */
			World1 = 161, /* non-US #1 */
			World2 = 162,  /* non-US #2 */


			// Function keys
			Escape = 256,
			Enter = 257,
			Tab = 258,
			Backspace = 259,
			Insert = 260,
			Delete = 261,
			Right = 262,
			Left = 263,
			Down = 264,
			Up = 265,
			PageUp = 266,
			PageDown = 267,
			Home = 268,
			End = 269,
			CapsLock = 280,
			ScrollLock = 281,
			NumLock = 282,
			PrintScreen = 283,
			Pause = 284,
			F1 = 290,
			F2 = 291,
			F3 = 292,
			F4 = 293,
			F5 = 294,
			F6 = 295,
			F7 = 296,
			F8 = 297,
			F9 = 298,
			F10 = 299,
			F11 = 300,
			F12 = 301,
			F13 = 302,
			F14 = 303,
			F15 = 304,
			F16 = 305,
			F17 = 306,
			F18 = 307,
			F19 = 308,
			F20 = 309,
			F21 = 310,
			F22 = 311,
			F23 = 312,
			F24 = 313,
			F25 = 314,
			NumPad0 = 320,
			NumPad1 = 321,
			NumPad2 = 322,
			NumPad3 = 323,
			NumPad4 = 324,
			NumPad5 = 325,
			NumPad6 = 326,
			NumPad7 = 327,
			NumPad8 = 328,
			NumPad9 = 329,
			KP_Decimal = 330,
			KP_Divide = 331,
			KP_Multiply = 332,
			KP_Subtract = 333,
			KP_Add = 334,
			KP_Enter = 335,
			KP_Equal = 336,
			LeftShift = 340,
			LeftControl = 341,
			LeftAlt = 342,
			LeftSuper = 343,
			RightShift = 344,
			RightControl = 345,
			RightAlt = 346,
			RightSuper = 347,
			Menu = 348,
		};
	}
	
}
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
