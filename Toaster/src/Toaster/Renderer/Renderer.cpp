#include "tstpch.h"

#include "Renderer.hpp"

#include "RenderCommand.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "platform/OpenGl/OpenGLShader.hpp"

namespace tst
{

	Renderer::SceneData* Renderer::m_sceneData = new Renderer::SceneData;

	void Renderer::init()
	{
		TST_PROFILE_FN();

		RenderCommand::init();
#ifdef TST_RENDER_2D
		Renderer2D::init();
#endif
#ifdef TST_RENDER_3D
		Renderer3D::init();
#endif
	}

	void Renderer::terminate()
	{
#ifdef TST_RENDER_2D
		Renderer2D::terminate();
#endif
#ifdef TST_RENDER_3D
		Renderer3D::terminate();
#endif
	}



	void Renderer::begin(const RefPtr<Camera>& camera)
	{
		m_sceneData->projectionMatrix = camera->getProjectionMatrix();
		m_sceneData->viewMatrix		  = camera->getViewMatrix();
	}
	void Renderer::begin(const RefPtr<PerspectiveCamera>& camera)
	{
		m_sceneData->projectionMatrix = camera->getProjectionMatrix();
		m_sceneData->viewMatrix		  = camera->getViewMatrix();
	}
	void Renderer::begin(const RefPtr<OrthoCamera>& camera)
	{
		m_sceneData->projectionMatrix = camera->getProjectionMatrix();
		m_sceneData->viewMatrix		  = camera->getViewMatrix();
	}
	void Renderer::begin(const RefPtr<OrthoCamera2D>& camera)
	{
		m_sceneData->projectionMatrix = camera->getProjectionMatrix();
		m_sceneData->viewMatrix		  = camera->getViewMatrix();
	}


	void Renderer::end()
	{
		
	}

	void Renderer::submit(const RefPtr<VertexArray> &vertexArray, const RefPtr<Shader>& shader, const glm::mat4 &transform = glm::mat4(1.0f))
	{
		shader->bind();

		if (dynamic_cast<OpenGLShader*>(shader.get()))
		{
			dynamic_cast<OpenGLShader*>(shader.get())->uploadUniformMatrix4f(transform, "u_Transform");
			dynamic_cast<OpenGLShader*>(shader.get())->uploadUniformMatrix4f(m_sceneData->projectionMatrix, "u_Projection");
			dynamic_cast<OpenGLShader*>(shader.get())->uploadUniformMatrix4f(m_sceneData->viewMatrix, "u_View");
		}
		else
		{
			TST_ASSERT(false, "Shader is not of type OpenGLShader!");
		}


		vertexArray->bind();
		RenderCommand::drawIndexed(vertexArray);
	}

	void Renderer::resizeViewport(uint32_t width, uint32_t height)
	{
		RenderCommand::resizeViewport(width, height);
	}


}
