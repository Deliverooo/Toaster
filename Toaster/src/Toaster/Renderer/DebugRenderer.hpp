#pragma once
#include "Camera.hpp"
#include "Mesh.hpp"

namespace tst {

	class DebugRenderer
	{
	public:

		static void init();
		static void terminate() noexcept;

		static void begin(const Camera& camera, const glm::mat4& transform);
		static void begin(const EditorCamera& camera);
		static void end();

		static void drawMeshBoundingBox(const RefPtr<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& colour = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		static void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour = glm::vec4(1.0f));

		static void drawBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
		static void drawWireCube(const glm::vec3& center, const glm::vec3& size, const glm::vec4& colour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		struct Stats
		{
			uint32_t linesDrawn = 0;
			uint32_t boxesDrawn = 0;
			uint32_t drawCalls = 0;
		};

		static Stats getStats();
		static void resetStats();

	private:
		static void flush();
		static void drawBatch();
	};
}
