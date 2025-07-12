#include "Toaster.h"

class TestLayer : public tst::Layer
{
public:
	TestLayer()
	{
	}

	void onUpdate() override
	{
		
	}

	void onEvent(tst::Event& e) override
	{
		if (e.getEventType() == tst::EventType::KeyPressed)
		{
			tst::KeyPressedEvent& key_pressed_event = static_cast<tst::KeyPressedEvent&>(e);
			TST_TRACE("{0}", static_cast<char>(key_pressed_event.getKeycode()));
		}
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