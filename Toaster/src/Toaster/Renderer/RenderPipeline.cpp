#include "tstpch.h"
#include "RenderPipeline.hpp"

namespace tst
{
	//RenderPipeline::RenderPipeline()
	//{
	//	// Initialize the render pipeline
	//	m_renderQueue = std::make_unique<RenderQueue>();
	//	m_cullingSystem = std::make_unique<CullingSystem>();
	//}
	//RenderPipeline::~RenderPipeline()
	//{
	//	// Cleanup resources
	//	m_renderQueue.reset();
	//	m_cullingSystem.reset();
	//}
	//void RenderPipeline::submitMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, uint32_t entityId)
	//{
	//	m_renderQueue->submitMesh(mesh, transform, entityId, m_cameraPosition);
	//}
	//void RenderPipeline::render()
	//{
	//	// Perform culling
	//	m_cullingSystem->performCulling(m_renderQueue->getCommands(), m_cameraPosition);
	//	// Sort and render the commands
	//	m_renderQueue->sortCommands();
	//	m_renderQueue->executeCommands();
	//}
}