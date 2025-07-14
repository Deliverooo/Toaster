#pragma once
#include "Toaster/Renderer/RenderingContext.hpp"
#include "Glad/glad.h"
#include "GLFW/glfw3.h"

namespace tst
{
	class TST_API OpenGLRenderingContext : public RenderingContext
	{
	public:

		OpenGLRenderingContext(GLFWwindow *window);
;		virtual ~OpenGLRenderingContext() override;


		virtual void init() override;
		virtual void swapBuffers() override;

	private:
		GLFWwindow* m_window;

	};
}
