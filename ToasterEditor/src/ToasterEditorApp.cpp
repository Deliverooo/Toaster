#include <random>

#include "Toaster.h"
// Toaster Engine Entry Point
#include "Toaster/Core/EntryPoint.hpp"

#include "imgui.h"
#include "ToasterEditorLayer.hpp"


namespace tst {

	class ToasterEditorApp : public Application
	{
	public:

		ToasterEditorApp() {
			pushLayer(std::make_shared<ToasterEditorLayer>());
		}

		~ToasterEditorApp() {

		}
	};

	// the definition of the CreateApplication function
	Application* CreateApplication() {
		TST_CORE_INFO("Created Editor Application!");

		return new ToasterEditorApp();
	}
}
