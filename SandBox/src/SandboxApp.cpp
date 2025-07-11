#include "Toaster.h"

class TestLayer : public tst::Layer
{
public:
	TestLayer()
	{
	}

	void onUpdate() override
	{
		//TST_INFO("Layer Updating!");
	}

	void onEvent(tst::Event& e) override
	{
		TST_TRACE("{0}", e);
	}
};

class SandboxApp : public tst::Application
{
public:
	SandboxApp() {
		pushLayer(new TestLayer());
		pushLayer(new tst::OpenGlTestLayer());
		pushOverlay(new tst::ImguiLayer());
	}
	~SandboxApp() {
		// Destructor implementation
	}
};

// the definition of the CreateApplication function
tst::Application* tst::CreateApplication() {
	std::cout << "Created app" << "\n";
	return new SandboxApp();
}