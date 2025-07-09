#pragma once

#include "Core.hpp"
#include "Events/Event.hpp"
#include <iostream>

namespace tst {

	class TST_API Application
	{
	public:
		Application();
		~Application();

		void run();
	};

	// To be defined in CLIENT
	extern Application* CreateApplication();

}
