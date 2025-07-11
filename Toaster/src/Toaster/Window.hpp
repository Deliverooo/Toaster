#pragma once
#include "tstpch.h"

#include "Events/Event.hpp"

namespace tst
{

	struct WindowAttribArray
	{
		unsigned int width{1280};
		unsigned int height{720};
		const char* title{ "Toaster" };

		//bool vSyncEnabled{ true };
	};

	class Window
	{
	public:

		static Window* Create(const WindowAttribArray &window_attributes = WindowAttribArray());
		virtual ~Window(){}
		
		virtual void update() = 0;

		virtual unsigned int getWidth() const = 0;
		virtual unsigned int getHeight() const = 0;
		virtual const char* getTitle() const = 0;

		virtual void setEventCallback(std::function<void(Event &)> e) = 0;

		//virtual bool getVsyncEnabled() const;


		//virtual void enableVsync(bool yn);
	};
}
