#pragma once
#include "Toaster/Core/Layer.hpp"
#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Events/KeyEvent.hpp"
#include "Toaster/Events/MouseEvent.hpp"
#include "imgui.h"

namespace tst
{
	struct TstFont
	{
		ImFont* font{ nullptr };
		bool isLoaded{ false };
		bool isPoped{ false };
		const char* fontFilepath{ "" };
	};


	class TST_API ImguiLayer : public Layer
	{
	public:
		ImguiLayer();
		~ImguiLayer() = default;

		virtual void onAttach() override;
		virtual void onDetach() override;
		virtual void onImguiRender() override{};

		void begin();
		void end();

	private:

		bool m_embededWindow = true;

		//ImFont* m_font;
	};
}
