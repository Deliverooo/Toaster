#include <random>

#include "Toaster.h"
// Toaster Engine Entry Point
#include "Toaster/Core/EntryPoint.hpp"

#include "imgui.h"
#include "Sandbox2D.hpp"
#include "Sandbox3D.hpp"


class SandboxApp : public tst::Application
{
public:
	SandboxApp() {

#ifdef TST_RENDER_2D
		pushLayer(std::make_shared<SandBox2DLayer>());
#endif
#ifdef TST_RENDER_3D
		pushLayer(std::make_shared<SandBox3DLayer>());
#endif

	}

	~SandboxApp() {

	}
};

// the definition of the CreateApplication function
tst::Application* tst::CreateApplication() {
	std::cout << "Created app" << "\n";

	return new SandboxApp();
}