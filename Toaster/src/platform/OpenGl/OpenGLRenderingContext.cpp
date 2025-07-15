#include "tstpch.h"
#include "OpenGLRenderingContext.hpp"


namespace tst {

	OpenGLRenderingContext::OpenGLRenderingContext(GLFWwindow* window) : m_window(window)
	{
		
	}

	OpenGLRenderingContext::~OpenGLRenderingContext()
	{
		
	}

	void OpenGLRenderingContext::init()
	{
		glfwMakeContextCurrent(m_window);

		int success = gladLoadGL();
		TST_ASSERT(success, "Glad Failed To Load and Initialize OpenGL!");

		TST_CORE_INFO("Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		TST_CORE_INFO("OpenGL Version: {0}", (const char*)glGetString(GL_VERSION));
		TST_CORE_INFO("GLSL Version: {0}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));


	}

	void OpenGLRenderingContext::swapBuffers()
	{
		glfwSwapBuffers(m_window);
	}




}