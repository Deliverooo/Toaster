#pragma once
#include "tstpch.h"
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
		virtual void onUpdate();
		virtual void onEvent(Event &e);
	};
}
