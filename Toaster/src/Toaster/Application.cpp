#include "Application.hpp"
#include "Events/ApplicationEvent.hpp"
#include "Events/KeyEvent.hpp"
#include "Log.hpp"

namespace tst
{
	Application::Application() {
		// Constructor implementation
	}

	Application::~Application() {
		// Destructor implementation
	}

	void Application::run() {
	
		KeyPressedEvent kev(VK_LEFT, false);

		if (kev.inCategory(EventCategoryApplication)) {
			TST_TRACE(kev);
		}
		if(kev.inCategory(EventCategoryInput)) {
			TST_TRACE(kev);
		}

		std::cin.get();
	}

}
