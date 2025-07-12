#pragma once
#include "Toaster/Input.hpp"

#ifdef TST_PLATFORM_WINDOWS
namespace tst
{
	class WindowsInput : public Input
	{
	protected:
		bool isKeyPressedPlatformNative(TstKeycode keycode) override;
		bool isMouseButtonPressedPlatformNative(TstMouseButton button) override;

		double getMouseX_PlatformNative() override;
		double getMouseY_PlatformNative() override;
		TstMousePos getMousePosPlatformNative() override;
	};

}
#endif
