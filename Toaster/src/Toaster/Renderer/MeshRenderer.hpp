#pragma once
#include "Camera.hpp"
#include "Light.hpp"
#include "Mesh.hpp"

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

		// Mesh rendering - Add these declarations
		static void drawMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform);
		static void drawMesh(const RefPtr<Mesh>& mesh, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);


		struct Stats
		{
			uint32_t drawCallCount{ 0 };
			uint32_t quadCount{ 0 };
			uint32_t triangleCount{ 0 };
			uint32_t textureBindings{ 0 };
			uint32_t verticesSubmitted{ 0 };
			uint32_t meshCount{ 0 };

			uint32_t totalVertexCount() { return quadCount * 4 + triangleCount * 3; }
			uint32_t totalIndexCount() { return quadCount * 6 + triangleCount * 6; }
			float batchEfficiency() { return drawCallCount > 0 ? static_cast<float>(quadCount) / static_cast<float>(drawCallCount) : 0.0f; }
		};

		static Stats getStats();
		static void resetStats();

	};
}