#pragma once
#include "tstpch.h"

#include "Core/Time.hpp"
#include "Events/Event.hpp"

namespace tst
{
	class TST_API Layer
	{
	public:
		Layer();
		virtual ~Layer();

		virtual void onAttach();
		virtual void onDetach();
		virtual void onUpdate(DeltaTime dt);
		virtual void onEvent(Event &e);
		virtual void onImguiRender(){};
	};
}
