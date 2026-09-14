#pragma once

#include <bitset>
#include <unordered_map>

#include "toast_kernel.hpp"

namespace toaster
{
	// Keycode numbers from <GLFW/glfw3.h>
	enum class EKeyCode : uint32
	{
		eSpace        = 32,
		eApostrophe   = 39,
		eComma        = 44,
		eMinus        = 45,
		ePeriod       = 46,
		eSlash        = 47,
		e0            = 48,
		e1            = 49,
		e2            = 50,
		e3            = 51,
		e4            = 52,
		e5            = 53,
		e6            = 54,
		e7            = 55,
		e8            = 56,
		e9            = 57,
		eSemicolon    = 59,
		eEqual        = 61,
		eA            = 65,
		eB            = 66,
		eC            = 67,
		eD            = 68,
		eE            = 69,
		eF            = 70,
		eG            = 71,
		eH            = 72,
		eI            = 73,
		eJ            = 74,
		eK            = 75,
		eL            = 76,
		eM            = 77,
		eN            = 78,
		eO            = 79,
		eP            = 80,
		eQ            = 81,
		eR            = 82,
		eS            = 83,
		eT            = 84,
		eU            = 85,
		eV            = 86,
		eW            = 87,
		eX            = 88,
		eY            = 89,
		eZ            = 90,
		eLeftBracket  = 91,
		eBackslash    = 92,
		eRightBracket = 93,
		eGraveAccent  = 96,
		eWorld1       = 161,
		eWorld2       = 162,
		eEscape       = 256,
		eEnter        = 257,
		eTab          = 258,
		eBackspace    = 259,
		eInsert       = 260,
		eDelete       = 261,
		eRight        = 262,
		eLeft         = 263,
		eDown         = 264,
		eUp           = 265,
		ePageUp       = 266,
		ePageDown     = 267,
		eHome         = 268,
		eEnd          = 269,
		eCapsLock     = 280,
		eScrollLock   = 281,
		eNumLock      = 282,
		ePrintScreen  = 283,
		ePause        = 284,
		eF1           = 290,
		eF2           = 291,
		eF3           = 292,
		eF4           = 293,
		eF5           = 294,
		eF6           = 295,
		eF7           = 296,
		eF8           = 297,
		eF9           = 298,
		eF10          = 299,
		eF11          = 300,
		eF12          = 301,
		eF13          = 302,
		eF14          = 303,
		eF15          = 304,
		eF16          = 305,
		eF17          = 306,
		eF18          = 307,
		eF19          = 308,
		eF20          = 309,
		eF21          = 310,
		eF22          = 311,
		eF23          = 312,
		eF24          = 313,
		eF25          = 314,
		eKP0          = 320,
		eKP1          = 321,
		eKP2          = 322,
		eKP3          = 323,
		eKP4          = 324,
		eKP5          = 325,
		eKP6          = 326,
		eKP7          = 327,
		eKP8          = 328,
		eKP9          = 329,
		eKPDecimal    = 330,
		eKPDivide     = 331,
		eKPMultiply   = 332,
		eKPSubtract   = 333,
		eKPAdd        = 334,
		eKPEnter      = 335,
		eKPEqual      = 336,
		eLeftShift    = 340,
		eLeftControl  = 341,
		eLeftAlt      = 342,
		eLeftSuper    = 343,
		eRightShift   = 344,
		eRightControl = 345,
		eRightAlt     = 346,
		eRightSuper   = 347,
		eMenu         = 348
	};

	enum class EMouseButton : int32
	{
		eButton1 = 0,
		eButton2 = 1,
		eButton3 = 2,
		eButton4 = 3,
		eButton5 = 4,
		eButton6 = 5,
		eButton7 = 6,
		eButton8 = 7,
		eLeft    = eButton1,
		eRight   = eButton2,
		eMiddle  = eButton3
	};

	enum class ECursorMode
	{
		eNormal,
		eHidden,
		eDisabled,
		eCaptured
	};

	class TST_KERNEL_API InputContext
	{
	public:
		InputContext() = default;

		explicit InputContext(void *p_window_ctx) : m_windowCtx(p_window_ctx)
		{
		}

		auto setWindowCtx(void *p_window_ctx) -> void { m_windowCtx = p_window_ctx; }

		// Keyboard
		auto isKeyDown(EKeyCode p_key_code) const -> bool;
		auto isKeyPressed(EKeyCode p_key_code) const -> bool;
		auto isKeyReleased(EKeyCode p_key_code) const -> bool;

		// Mouse
		auto isMouseButtonDown(EMouseButton p_button) const -> bool;
		auto isMouseButtonPressed(EMouseButton p_button) const -> bool;
		auto isMouseButtonReleased(EMouseButton p_button) const -> bool;

		auto setCursorMode(ECursorMode p_mode) -> void;
		auto getCursorMode() const -> ECursorMode { return m_cursorMode; }

		auto getMouseX() const -> float32 { return m_mouseX; }
		auto getMouseY() const -> float32 { return m_mouseY; }
		auto getMouseDx() const -> float32 { return m_mouseDx; }
		auto getMouseDy() const -> float32 { return m_mouseDy; }

		auto getScrollX() const -> float32 { return m_scrollX; }
		auto getScrollY() const -> float32 { return m_scrollY; }

		auto update() -> void;
		auto clearStates() -> void;

	private:
		std::unordered_map<EKeyCode, bool> m_currentKeyStates;
		std::unordered_map<EKeyCode, bool> m_previousKeyStates;

		std::unordered_map<EMouseButton, bool> m_currentMouseStates;
		std::unordered_map<EMouseButton, bool> m_previousMouseStates;

		void *m_windowCtx{nullptr}; // GLFWwindow*

		ECursorMode m_cursorMode{ECursorMode::eNormal};

		// Essentially, when the cursor first enters the window, the initial X position will be out of date. This will mess up the Dx/Dy and cause issues...
		bool m_firstMouse{true};

		float32 m_mouseX{0.0f};
		float32 m_mouseY{0.0f};
		float32 m_mouseDx{0.0f};
		float32 m_mouseDy{0.0f};

		float32 m_scrollX{0.0f};
		float32 m_scrollY{0.0f};

		friend class Window;
	};
}
