#include "tstpch.h"

#include "Renderer.hpp"

#include "RenderCommand.hpp"

namespace tst
{

	Renderer::SceneData* Renderer::m_sceneData = new Renderer::SceneData;

	void Renderer::begin(const std::shared_ptr<Camera>& camera)
	{
		m_sceneData->projectionMatrix = camera->getProjectionMatrix();
		m_sceneData->viewMatrix = camera->getViewMatrix();
	}

	//void Renderer::begin(const std::shared_ptr<Camera>& camera)
	//{
	//	m_sceneData->projectionMatrix = camera->getProjectionMatrix();
	//	m_sceneData->viewMatrix = camera->getViewMatrix();
	//}

	void Renderer::end()
	{
		
	}

	void Renderer::submit(const std::shared_ptr<VertexArray> &vertexArray, const glm::mat4 &transform, const std::shared_ptr<Shader> &shader)
	{
		shader->bind();

		shader->uploadUniformMatrix4f(transform, "transform");
		shader->uploadUniformMatrix4f(m_sceneData->projectionMatrix, "projection");
		shader->uploadUniformMatrix4f(m_sceneData->viewMatrix, "view");

		vertexArray->bind();
		RenderCommand::drawIndexed(vertexArray);
	}

}
