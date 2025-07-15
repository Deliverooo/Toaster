#pragma once

#ifdef TST_PLATFORM_WINDOWS

// Toaster Engine Entry Point
// This file is included in the main application file to define the entry point for the Toaster engine.
extern tst::Application* tst::CreateApplication();


// the main function for the Toaster engine application
int main() {

	// Initializes the logging system
	tst::Log::init();

	// Create an instance of the application and run it
	// the instance is heap allocated because we will need to allocate a lot of resources
	auto app = tst::CreateApplication();
	app->run();
	delete app;


	return 0;
}

#endif