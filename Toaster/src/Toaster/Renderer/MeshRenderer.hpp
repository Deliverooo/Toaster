#pragma once
#include "Camera.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "RenderQueue.hpp"

namespace tst
{
	class TST_API MeshRenderer
	{
	public:


		static void init();
		static void terminate() noexcept;

		static void begin(const Camera& camera, const glm::mat4& transform);
		static void begin(const EditorCamera& camera);
		static void end();

		static void uploadLightingData(const Light& light, const glm::vec3& lightPosition, const glm::vec3& lightDirection);
		static void flushLights();

		static void flush();

		// Mesh rendering - Add these declarations
		static void drawMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform);
		static void drawMesh(const RefPtr<Mesh>& mesh, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);

		static void submitMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, uint32_t entityId);

		struct Stats
		{
			uint32_t drawCallCount = 0;
			uint32_t triangleCount = 0;
			uint32_t textureBindings = 0;
			uint32_t verticesSubmitted = 0;
			uint32_t meshCount = 0;
			uint32_t culledObjects = 0;
			uint32_t totalSubmitted = 0;
		};

		static Stats getStats();
		static void resetStats();


		struct MaterialBatch
		{
			RefPtr<Material> material;
			std::vector<SubMeshRenderCommand> commands;
		};

	private:

		static void renderSubmesh(const SubMeshRenderCommand& command);

		static void renderPass(RenderPass pass);
		static void groupByMaterial(const std::vector<SubMeshRenderCommand>& commands);
		static void renderMaterialBatch(const MaterialBatch& batch);

	};
}
