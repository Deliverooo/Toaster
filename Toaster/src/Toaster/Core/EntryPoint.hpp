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
	TST_PROFILE_BEGIN("Start", "Tst_Prof-Startup.json");
	auto app = tst::CreateApplication();
	TST_PROFILE_END();

	TST_PROFILE_BEGIN("Run", "Tst_Prof-Run.json");
	app->run();
	TST_PROFILE_END();

	TST_PROFILE_BEGIN("Termination", "Tst_Prof-Termination.json");
	delete app;
	TST_PROFILE_END();

	return 0;
}

#endif