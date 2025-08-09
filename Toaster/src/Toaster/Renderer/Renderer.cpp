#include "tstpch.h"

#include "Renderer.hpp"

#include "RenderCommand.hpp"
#include "Renderer2D.hpp"
#include "SkyBoxRenderer.hpp"
#include "platform/OpenGl/OpenGLShader.hpp"
#include "Toaster/Util/MathUtil.hpp"

namespace tst
{
	bool Renderer::s_initialized = false;
	RendererStats Renderer::s_stats;

	void Renderer::init()
	{
		GraphicsAPI::init();
		MeshRenderer::init();
		Renderer2D::init();
		SkyBoxRenderer::init();
		s_initialized = true;
	}

	void Renderer::shutdown()
	{
		SkyBoxRenderer::terminate();
		Renderer2D::terminate();
		MeshRenderer::terminate();
	}

	RendererStats Renderer::getStats() { return s_stats; }

	void Renderer::resetStats()
	{
		s_stats.drawCallCount = 0;
		s_stats.triangleCount = 0;
		s_stats.textureBindings = 0;
		s_stats.verticesSubmitted = 0;
		s_stats.meshCount = 0;
		s_stats.culledObjects = 0;
		s_stats.totalSubmitted = 0;
	}

	void Renderer::beginScene(const Camera& camera, const glm::mat4& transform)
	{
		
	}

	void Renderer::beginScene(const EditorCamera& camera)
	{
		MeshRenderer::begin(camera);
		//Renderer2D::begin(camera);
	}

	/*RenderSubmission Renderer::draw(const RefPtr<Mesh>& mesh)
	{
		return RenderSubmission(mesh);
	}

	void Renderer::drawMesh(RefPtr<Mesh> mesh, const glm::mat4& transform)
	{
		MeshRenderer::drawMesh(mesh, transform);
	}

	void Renderer::submit(const RenderCommand& command, RenderPass pass)
	{
		MeshRenderer::submitCustomCommand(command, pass);
	}*/

	void Renderer::drawQuad(const glm::mat4& transform, const glm::vec4& colour)
	{
		Renderer2D::drawQuad(transform, colour);
	}

	void Renderer::endScene()
	{
		MeshRenderer::flush();
		MeshRenderer::end();
		//Renderer2D::end();

	}

}
