#pragma once
#include "glm/ext/matrix_transform.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshRenderer.hpp"
#include "RenderQueue.hpp"
#include "RendererAPI.hpp"
#include "RenderSubmission.hpp"
#include "Shader.hpp"
#include "Texture.hpp"


namespace tst
{

    

    struct RendererStats
    {
        uint32_t drawCallCount = 0;
        uint32_t triangleCount = 0;
        uint32_t textureBindings = 0;
        uint32_t verticesSubmitted = 0;
        uint32_t meshCount = 0;
        uint32_t culledObjects = 0;
        uint32_t totalSubmitted = 0;

	};

    class TST_API Renderer
    {
    public:

        static void init();
        static void shutdown();

        static void beginScene(const EditorCamera& camera);
        static void beginScene(const Camera& camera, const glm::mat4& transform);
        static void endScene();

        //static RenderSubmission draw(const RefPtr<Mesh>& mesh);
        //static void submit(const RenderCommand& command, RenderPass pass = RenderPass::Opaque);

        // Direct rendering methods (for immediate mode)
        static void drawMesh(RefPtr<Mesh> mesh, const glm::mat4& transform);
        static void drawQuad(const glm::mat4& transform, const glm::vec4& colour);

        static RendererStats getStats();
        static void resetStats();


    private:
        static bool s_initialized;
		static RendererStats s_stats;
    };
}
