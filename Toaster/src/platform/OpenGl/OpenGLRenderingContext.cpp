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

#ifdef _DEBUG
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
			GLsizei length, const GLchar* message, const void* userParam) {

				std::string sourceStr;
				switch (source) {
				case GL_DEBUG_SOURCE_API:				  sourceStr = "API"; break;
				case GL_DEBUG_SOURCE_WINDOW_SYSTEM:       sourceStr = "WINDOW SYSTEM"; break;
				case GL_DEBUG_SOURCE_SHADER_COMPILER:     sourceStr = "SHADER COMPILATION"; break;
				case GL_DEBUG_SOURCE_THIRD_PARTY:		  sourceStr = "THIRD PARTY"; break;
				case GL_DEBUG_SOURCE_APPLICATION:		  sourceStr = "APPLICATION"; break;
				case GL_DEBUG_SOURCE_OTHER:				  sourceStr = "OTHER"; break;
				}

				std::string severityStr;
				switch (severity) {
				case GL_DEBUG_SEVERITY_HIGH:         severityStr = "HIGH"; break;
				case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "MEDIUM"; break;
				case GL_DEBUG_SEVERITY_LOW:          severityStr = "LOW"; break;
				case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "NOTIFICATION"; break;
				}
				std::string typeStr;
				switch (type) {
				case GL_DEBUG_TYPE_ERROR:               typeStr = "ERROR"; break;
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "DEPRECATED"; break;
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "UNDEFINED"; break;
				case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "PORTABILITY"; break;
				case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "PERFORMANCE"; break;
				case GL_DEBUG_TYPE_OTHER:               typeStr = "OTHER"; break;
				}

				TST_CORE_ERROR("OpenGL Debug ({0}|{1}) ({2}): {3}", severityStr, typeStr, sourceStr, message);
			}, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

#endif
	}

	void OpenGLRenderingContext::swapBuffers()
	{
		TST_PROFILE_FN();

		glfwSwapBuffers(m_window);
	}




}