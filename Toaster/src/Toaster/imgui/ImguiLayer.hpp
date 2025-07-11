#pragma once
#include "Toaster/Layer.hpp"

namespace tst
{
	class TST_API ImguiLayer : public Layer
	{
	public:
		ImguiLayer();
		~ImguiLayer();

		void onAttach() override;
		void onDetach() override;
		void onUpdate() override;
		void onEvent(Event& e) override;

	private:
		float m_time = 0.0f;
	};
}
