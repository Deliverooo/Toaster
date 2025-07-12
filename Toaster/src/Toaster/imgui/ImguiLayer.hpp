#pragma once
#include "Toaster/Layer.hpp"
#include "Toaster/Events/ApplicationEvent.hpp"
#include "Toaster/Events/KeyEvent.hpp"
#include "Toaster/Events/MouseEvent.hpp"

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

		bool onKeyPressedEvent(KeyPressedEvent &e);
		bool onKeyReleasedEvent(KeyReleasedEvent &e);
		bool onKeyHeldEvent(KeyHeldEvent &e);
		bool onKeyTypedEvent(KeyTypedEvent &e);

		bool onMouseButtonPressedEvent(MouseButtonPressEvent& e);
		bool onMouseButtonReleasedEvent(MouseButtonReleaseEvent& e);
		bool onMouseButtonHeldEvent(MouseButtonHeldEvent& e);

		bool onMouseMovedEvent(MouseMoveEvent& e);
		bool onMouseScrollEvent(MouseScrollEvent& e);


		bool onWindowResizeEvent(WindowResizedEvent& e);
		bool onWindowClosedEvent(WindowClosedEvent& e);
		bool onWindowMaximizedEvent(WindowMaximizedEvent& e);
		bool onWindowLostFocusEvent(WindowLostFocusEvent& e);
		bool onWindowGainFocusEvent(WindowLostFocusEvent& e);


		float m_time = 0.0f;
	};
}
