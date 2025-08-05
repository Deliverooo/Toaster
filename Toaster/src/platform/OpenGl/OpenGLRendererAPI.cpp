#include "tstpch.h"
#include "OpenGLRendererAPI.hpp"
#include <glad/glad.h>

namespace tst
{
	void OpenGLRendererAPI::init()
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void OpenGLRendererAPI::clear()
	{
		TST_PROFILE_FN();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::setClearColour(const glm::vec4& colour)
	{
		TST_PROFILE_FN();

		glClearColor(colour.r, colour.g, colour.b, colour.a);
	}

	void OpenGLRendererAPI::disableDepthTesting()
	{
		glDisable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::enableDepthTesting()
	{
		glEnable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::setDepthFunc(DepthFunc func)
	{
		switch (func)
		{
		case DepthFunc::Always:
			glDepthFunc(GL_ALWAYS);
			break;
		case DepthFunc::Never:
			glDepthFunc(GL_NEVER);
			break;
		case DepthFunc::Less:
			glDepthFunc(GL_LESS);
			break;
		case DepthFunc::LessOrEqual:
			glDepthFunc(GL_LEQUAL);
			break;
		case DepthFunc::Greater:
			glDepthFunc(GL_GREATER);
			break;
		case DepthFunc::GreaterOrEqual:
			glDepthFunc(GL_GEQUAL);
			break;
		case DepthFunc::Equal:
			glDepthFunc(GL_EQUAL);
			break;
		case DepthFunc::NotEqual:
			glDepthFunc(GL_NOTEQUAL);
			break;
		default:
			TST_ASSERT(false, "Unknown depth function");
			break;
		}
	}

//	void OpenGLRendererAPI::validateShaderProgram(uint32_t programId, const std::string& name)
//	{
//#ifdef _DEBUG
//		GLint isLinked = 0;
//		glGetProgramiv(programId, GL_LINK_STATUS, &isLinked);
//		if (isLinked == GL_FALSE) {
//			GLint maxLength = 0;
//			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &maxLength);
//
//			std::vector<GLchar> infoLog(maxLength);
//			glGetProgramInfoLog(programId, maxLength, &maxLength, &infoLog[0]);
//
//			TST_CORE_ERROR("Shader program '{0}' failed to link: {1}", name, infoLog.data());
//		}
//#endif
//	}
//
//	void OpenGLRendererAPI::validateFramebuffer()
//	{
//#ifdef _DEBUG
//		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//		if (status != GL_FRAMEBUFFER_COMPLETE) {
//			std::string error;
//			switch (status) {
//			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: error = "INCOMPLETE_ATTACHMENT"; break;
//			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: error = "MISSING_ATTACHMENT"; break;
//			case GL_FRAMEBUFFER_UNSUPPORTED: error = "UNSUPPORTED"; break;
//			default: error = "Unknown error " + std::to_string(status); break;
//			}
//			TST_CORE_ERROR("Framebuffer incomplete: {0}", error);
//		}
//#endif
//	}

	void OpenGLRendererAPI::enableBackfaceCulling()
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void OpenGLRendererAPI::enableDepthMask()
	{
		glDepthMask(GL_TRUE);
	}

	void OpenGLRendererAPI::disableDepthMask()
	{
		glDepthMask(GL_FALSE);
	}


	void OpenGLRendererAPI::cleanState()
	{
		glUseProgram(0);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glActiveTexture(GL_TEXTURE0);
	}

	void OpenGLRendererAPI::disableBackfaceCulling()
	{
		glDisable(GL_CULL_FACE);
	}

	void OpenGLRendererAPI::checkError(const std::string& operation)
	{
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			std::string message;
			switch (error)
			{
			case GL_INVALID_ENUM:      message = "GL_INVALID_ENUM"; break;
			case GL_INVALID_VALUE:     message = "GL_INVALID_VALUE"; break;
			case GL_INVALID_OPERATION: message = "GL_INVALID_OPERATION"; break;
			case GL_OUT_OF_MEMORY:     message = "GL_OUT_OF_MEMORY"; break;
			case GL_STACK_UNDERFLOW:   message = "GL_STACK_UNDERFLOW"; break;
			default:                   message = "Unknown OpenGL error " + std::to_string(error); break;
			}
			TST_CORE_ERROR("OpenGL Error after {0}: {1}", operation, message);
		}
	}

	void OpenGLRendererAPI::drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count)
	{
		vertexArray->bind();

		uint32_t indexCount = (count == 0) ? vertexArray->getIndexBuffer()->count() : count;
		glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount), GL_UNSIGNED_INT, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLRendererAPI::drawIndexedBaseVertex(const RefPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexOffset, uint32_t baseVertex)
	{
		vertexArray->bind();

		// Calculate the byte offset for the indices
		size_t indexOffsetBytes = indexOffset * sizeof(uint32_t);

		// Use glDrawElementsBaseVertex for efficient submesh rendering
		glDrawElementsBaseVertex(
			GL_TRIANGLES,                                    // primitive type
			indexCount,                                      // number of indices to render
			GL_UNSIGNED_INT,                                // index type
			reinterpret_cast<void*>(indexOffsetBytes),      // offset into index buffer
			baseVertex                                       // base vertex offset
		);

		vertexArray->unbind();
	}

	void OpenGLRendererAPI::drawArrays(const RefPtr<VertexArray>& vertexArray, const uint32_t count)
	{
		glDrawArrays(GL_TRIANGLES, 0, count);
	}

	void OpenGLRendererAPI::resizeViewport(uint32_t width, uint32_t height)
	{
		glViewport(0, 0, width, height);
	}


}