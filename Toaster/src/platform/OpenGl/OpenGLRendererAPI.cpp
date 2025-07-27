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

	void OpenGLRendererAPI::drawIndexed(const RefPtr<VertexArray>& vertexArray, uint32_t count)
	{
		vertexArray->bind();

		uint32_t indexCount = (count == 0) ? vertexArray->getIndexBuffer()->count() : count;
		glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount), GL_UNSIGNED_INT, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLRendererAPI::drawArrays(const RefPtr<VertexArray>& vertexArray)
	{
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	void OpenGLRendererAPI::resizeViewport(uint32_t width, uint32_t height)
	{
		glViewport(0, 0, width, height);
	}


}