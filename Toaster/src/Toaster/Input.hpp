#pragma once
#include "Core.hpp"

using TstKeycode = int;
using TstKeyState = int;

using TstMouseButton = int;
using TstMouseButtonState = int;

struct TstMousePos
{
	double x, y;
};

namespace tst
{
	class TST_API Input
	{
	public:
		static bool isKeyPressed(TstKeycode keycode) { return m_instance->isKeyPressedPlatformNative(keycode); }
		static bool isMouseButtonPressed(TstMouseButton button) { return m_instance->isMouseButtonPressedPlatformNative(button); }
		static double getMouseX() { return m_instance->getMouseX_PlatformNative(); }
		static double getMouseY() { return m_instance->getMouseY_PlatformNative(); }
		static TstMousePos getMousePos() { return m_instance->getMousePosPlatformNative(); }

	private:
		static Input* m_instance;

	protected:

		virtual bool isKeyPressedPlatformNative(TstKeycode keycode) = 0;
		virtual bool isMouseButtonPressedPlatformNative(TstMouseButton button) = 0;
		virtual double getMouseX_PlatformNative() = 0;
		virtual double getMouseY_PlatformNative() = 0;
		virtual TstMousePos getMousePosPlatformNative() = 0;
	};
}
