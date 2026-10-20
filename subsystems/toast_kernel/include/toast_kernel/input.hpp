#pragma once

#include "toast_kernel.hpp"
#include "toast_lib/bit.h"
#include "toast_lib/enum.hpp"
#include "toast_math/math_vector.hpp"

namespace toaster::knl
{
	// Keycode numbers from <GLFW/glfw3.h>
	enum class EKeyCode : uint16
	{
		eSpace        = 32u,
		eApostrophe   = 39u,
		eComma        = 44u,
		eMinus        = 45u,
		ePeriod       = 46u,
		eSlash        = 47u,
		e0            = 48u,
		e1            = 49u,
		e2            = 50u,
		e3            = 51u,
		e4            = 52u,
		e5            = 53u,
		e6            = 54u,
		e7            = 55u,
		e8            = 56u,
		e9            = 57u,
		eSemicolon    = 59u,
		eEqual        = 61u,
		eA            = 65u,
		eB            = 66u,
		eC            = 67u,
		eD            = 68u,
		eE            = 69u,
		eF            = 70u,
		eG            = 71u,
		eH            = 72u,
		eI            = 73u,
		eJ            = 74u,
		eK            = 75u,
		eL            = 76u,
		eM            = 77u,
		eN            = 78u,
		eO            = 79u,
		eP            = 80u,
		eQ            = 81u,
		eR            = 82u,
		eS            = 83u,
		eT            = 84u,
		eU            = 85u,
		eV            = 86u,
		eW            = 87u,
		eX            = 88u,
		eY            = 89u,
		eZ            = 90u,
		eLeftBracket  = 91u,
		eBackslash    = 92u,
		eRightBracket = 93u,
		eGraveAccent  = 96u,
		eWorld1       = 161u,
		eWorld2       = 162u,
		eEscape       = 256u,
		eEnter        = 257u,
		eTab          = 258u,
		eBackspace    = 259u,
		eInsert       = 260u,
		eDelete       = 261u,
		eRight        = 262u,
		eLeft         = 263u,
		eDown         = 264u,
		eUp           = 265u,
		ePageUp       = 266u,
		ePageDown     = 267u,
		eHome         = 268u,
		eEnd          = 269u,
		eCapsLock     = 280u,
		eScrollLock   = 281u,
		eNumLock      = 282u,
		ePrintScreen  = 283u,
		ePause        = 284u,
		eF1           = 290u,
		eF2           = 291u,
		eF3           = 292u,
		eF4           = 293u,
		eF5           = 294u,
		eF6           = 295u,
		eF7           = 296u,
		eF8           = 297u,
		eF9           = 298u,
		eF10          = 299u,
		eF11          = 300u,
		eF12          = 301u,
		eF13          = 302u,
		eF14          = 303u,
		eF15          = 304u,
		eF16          = 305u,
		eF17          = 306u,
		eF18          = 307u,
		eF19          = 308u,
		eF20          = 309u,
		eF21          = 310u,
		eF22          = 311u,
		eF23          = 312u,
		eF24          = 313u,
		eF25          = 314u,
		eKP0          = 320u,
		eKP1          = 321u,
		eKP2          = 322u,
		eKP3          = 323u,
		eKP4          = 324u,
		eKP5          = 325u,
		eKP6          = 326u,
		eKP7          = 327u,
		eKP8          = 328u,
		eKP9          = 329u,
		eKPDecimal    = 330u,
		eKPDivide     = 331u,
		eKPMultiply   = 332u,
		eKPSubtract   = 333u,
		eKPAdd        = 334u,
		eKPEnter      = 335u,
		eKPEqual      = 336u,
		eLeftShift    = 340u,
		eLeftControl  = 341u,
		eLeftAlt      = 342u,
		eLeftSuper    = 343u,
		eRightShift   = 344u,
		eRightControl = 345u,
		eRightAlt     = 346u,
		eRightSuper   = 347u,
		eMenu         = 348u,
		eLast         = eMenu
	};

	enum class EMouseButton : uint8
	{
		eButton1 = 0u,
		eButton2 = 1u,
		eButton3 = 2u,
		eButton4 = 3u,
		eButton5 = 4u,
		eButton6 = 5u,
		eButton7 = 6u,
		eButton8 = 7u,
		eLeft    = eButton1,
		eRight   = eButton2,
		eMiddle  = eButton3,
		eLast    = eMiddle
	};

	enum class ECursorMode : uint8
	{
		eNormal,
		eHidden,
		eDisabled,
		eCaptured,
		eLast = eCaptured
	};

	struct TST_KERNEL_API InputContext
	{
		void *windowCtx; // GLFWwindow*

		TST_LIB_BITSET(currentKeyStates, TST_LIB_ENUM_VAL(EKeyCode::eLast));
		TST_LIB_BITSET(previousKeyStates, TST_LIB_ENUM_VAL(EKeyCode::eLast));

		TST_LIB_BITSET(currentMouseStates, TST_LIB_ENUM_VAL(EMouseButton::eLast));
		TST_LIB_BITSET(previousMouseStates, TST_LIB_ENUM_VAL(EMouseButton::eLast));

		float32 mouseX;
		float32 mouseY;
		float32 mouseDx;
		float32 mouseDy;

		float32 scrollX;
		float32 scrollY;

		ECursorMode currentCursorMode;

		// Essentially, when the cursor first enters the window, the initial X position will be out of date. This will mess up the Dx/Dy and cause issues...
		bool firstMouse;
	};

	void TST_KERNEL_API input_ctx_create(InputContext *p_out_input_ctx, void *p_window_ctx);
	void TST_KERNEL_API input_ctx_destroy(InputContext *p_input_ctx);

	bool TST_KERNEL_API input_is_key_down(const InputContext *p_input_ctx, EKeyCode p_key_code);
	bool TST_KERNEL_API input_is_key_pressed(const InputContext *p_input_ctx, EKeyCode p_key_code);
	bool TST_KERNEL_API input_is_key_released(const InputContext *p_input_ctx, EKeyCode p_key_code);

	bool TST_KERNEL_API input_is_mouse_button_down(const InputContext *p_input_ctx, EMouseButton p_button);
	bool TST_KERNEL_API input_is_mouse_button_pressed(const InputContext *p_input_ctx, EMouseButton p_button);
	bool TST_KERNEL_API input_is_mouse_button_released(const InputContext *p_input_ctx, EMouseButton p_button);

	ECursorMode TST_KERNEL_API input_get_cursor_mode(const InputContext *p_input_ctx);
	void TST_KERNEL_API        input_set_cursor_mode(InputContext *p_input_ctx, ECursorMode p_cursor_mode);

	void TST_KERNEL_API input_update(InputContext *p_input_ctx);
	void TST_KERNEL_API input_clear_states(InputContext *p_input_ctx);

	math::Float2 TST_KERNEL_API input_get_mouse_pos(const InputContext *p_input_ctx);
	math::Float2 TST_KERNEL_API input_get_mouse_delta_pos(const InputContext *p_input_ctx);
	math::Float2 TST_KERNEL_API input_get_mouse_scroll(const InputContext *p_input_ctx);
}
