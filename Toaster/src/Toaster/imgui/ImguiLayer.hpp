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
		virtual ~ImguiLayer() override{};

		virtual void onAttach() override;
		virtual void onDetach() override;
		virtual void onImguiRender() override{};

		virtual void onEvent(Event& e) override;

		void begin();
		void end();

		void setBlockEvents(const bool block) { m_BlockEvents = block; }

	private:

		bool m_BlockEvents = true;
	};
}
