#include "Toaster.h"
#include "imgui.h"

struct TstFont
{
	ImFont* font{nullptr};
	bool isLoaded{false};
	bool isPoped{false};
	const char* fontFilepath{""};
};

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

	void onImguiRender() override
	{
		ImGuiIO& io = ImGui::GetIO();

		static TstFont tstFonts[3] = {
			{nullptr, false, false, "C:\\Users\\oocon\\AppData\\Local\\Microsoft\\Windows\\Fonts\\JetBrainsMono-Medium.ttf"},
			{nullptr, false, false, "C:\\Users\\oocon\\AppData\\Local\\Microsoft\\Windows\\Fonts\\Overload.otf"},
			{nullptr, false, false, "C:\\Users\\oocon\\AppData\\Local\\Microsoft\\Windows\\Fonts\\Monocraft.ttf"}};


		for (int i = 0; i < 3; i++)
		{
			if (!tstFonts[i].isLoaded)
			{
				tstFonts[i].font = io.Fonts->AddFontFromFileTTF(tstFonts[i].fontFilepath, 16);
				ImGui::PushFont(tstFonts[i].font);
				tstFonts[i].isLoaded = true;
			}
		}


		for (int i = 0; i < 3; i++)
		{
			if (!tstFonts[i].isPoped)
			{
				ImGui::PopFont();
				tstFonts[i].isPoped = true;
			}
		}
	}
};

class SandboxApp : public tst::Application
{
public:
	SandboxApp() {
		pushLayer(new TestLayer());
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