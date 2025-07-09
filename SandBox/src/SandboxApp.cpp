#include "Toaster.h"

class SandboxApp : public tst::Application
{
public:
	SandboxApp() {
		// Constructor implementation
	}
	~SandboxApp() {
		// Destructor implementation
	}
};

// the definition of the CreateApplication function
tst::Application* tst::CreateApplication() {
	return new SandboxApp();
}