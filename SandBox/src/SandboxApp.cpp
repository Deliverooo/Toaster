#include <random>

#include "Toaster.h"
// Toaster Engine Entry Point
#include "Toaster/Core/EntryPoint.hpp"

#include "imgui.h"
#include "Sandbox2D.hpp"
#include "Toaster/CameraController.hpp"
#include "Sandbox3D.hpp"

class TestLayer : public tst::Layer
{
public:
	TestLayer()
	{


	}

};



class SandboxApp : public tst::Application
{
public:
	SandboxApp() {

		//pushLayer(std::make_shared<SandBox3DLayer>());
		pushLayer(std::make_shared<SandBox2DLayer>());
	}

	~SandboxApp() {

	}
};

// the definition of the CreateApplication function
tst::Application* tst::CreateApplication() {
	std::cout << "Created app" << "\n";

	return new SandboxApp();
}