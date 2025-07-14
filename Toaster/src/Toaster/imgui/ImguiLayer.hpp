#pragma once
#include "Toaster/Layer.hpp"
#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Events/KeyEvent.hpp"
#include "Toaster/Events/MouseEvent.hpp"
#include "imgui.h"

namespace tst
{
	class TST_API ImguiLayer : public Layer
	{
	public:
		ImguiLayer();
		~ImguiLayer();

		virtual void onAttach() override;
		virtual void onDetach() override;
		virtual void onImguiRender() override;

		void begin();
		void end();

	private:

		bool m_embededWindow = true;

		//ImFont* m_font;
	};
}
