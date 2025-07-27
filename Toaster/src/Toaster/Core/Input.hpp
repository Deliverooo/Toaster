#pragma once
#include "Core.hpp"

using TstKeycode = int;
using TstKeyState = int;

using TstMouseButton = int;
using TstMouseButtonState = int;

using TstGamepadButton = int;
using TstGamepadButtonState = int;

struct TstMousePos
{
	double x, y;
};

namespace tst
{
	class TST_API Input
	{
	public:

		static bool isKeyPressed(TstKeycode keycode);
		static bool isMouseButtonPressed(TstMouseButton button);

		static double getMouseX();
		static double getMouseY();
		static std::pair<double, double> getMousePos();

		static void focusMouseCursor();
		static void unfocusMouseCursor();
		static bool isMouseCursorFocused();
	};
}
