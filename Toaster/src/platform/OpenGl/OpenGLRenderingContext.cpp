#include "tstpch.h"
#include "OpenGLRenderingContext.hpp"


namespace tst {

	OpenGLRenderingContext::OpenGLRenderingContext(GLFWwindow* window) : m_window(window)
	{
		
	}

	OpenGLRenderingContext::~OpenGLRenderingContext()
	{
		m_window = nullptr;
	}

	void OpenGLRenderingContext::init()
	{
		TST_PROFILE_FN();

		glfwMakeContextCurrent(m_window);

		int success = gladLoadGL();

		TST_ASSERT(success, "Glad Failed To Load and Initialize OpenGL!");

		TST_CORE_INFO("Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		TST_CORE_INFO("OpenGL Version: {0}", (const char*)glGetString(GL_VERSION));
		TST_CORE_INFO("GLSL Version: {0}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

		glEnable(GL_DEBUG_OUTPUT);
	}

	void OpenGLRenderingContext::swapBuffers()
	{
		TST_PROFILE_FN();

		glfwSwapBuffers(m_window);
	}




}