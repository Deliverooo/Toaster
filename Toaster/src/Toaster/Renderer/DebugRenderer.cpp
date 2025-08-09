#include "tstpch.h"
#include "DebugRenderer.hpp"

#include "RenderCommand.hpp"
#include "glad/glad.h"


namespace tst
{
    struct LineVertex
    {
        glm::vec3 position;
        glm::vec4 colour;

    };
    struct DebugRendererData
    {
        RefPtr<Shader> lineShader;
        RefPtr<VertexArray> lineVertexArray;
        RefPtr<VertexBuffer> lineVertexBuffer;

        glm::mat4 viewProjectionMatrix;

        static const uint32_t maxLines = 10000;
        static const uint32_t maxVertices = maxLines * 2;

        std::vector<LineVertex> lineVertices;

        DebugRenderer::Stats stats;
    };

    static DebugRendererData debug_data;

    void DebugRenderer::init()
    {

        debug_data.lineShader = Shader::create("LineShader",
            TST_CORE_RESOURCE_DIR"/shaders/LineShader.vert.glsl",
            TST_CORE_RESOURCE_DIR"/shaders/LineShader.frag.glsl");

        debug_data.lineVertexArray = VertexArray::create();
        debug_data.lineVertexBuffer = VertexBuffer::create(debug_data.maxVertices * sizeof(LineVertex));

        BufferLayout layout = {
            { "a_Position", ShaderDataType::Float3 },
            { "a_Colour",   ShaderDataType::Float4 }
        };

        debug_data.lineVertexBuffer->setLayout(layout);
        debug_data.lineVertexArray->addVertexBuffer(debug_data.lineVertexBuffer);

        debug_data.lineVertices.reserve(debug_data.maxVertices);

        TST_CORE_INFO("Debug Renderer initialized");
    }

    void DebugRenderer::terminate() noexcept
    {
	    TST_CORE_INFO("Debug Renderer terminated");
    }

    void DebugRenderer::begin(const Camera& camera, const glm::mat4& transform)
    {
        auto& projection = camera.getProjection();
        auto view = glm::inverse(transform);
        debug_data.viewProjectionMatrix = projection * view;

        debug_data.lineVertices.clear();
        debug_data.stats = {};
    }

    void DebugRenderer::begin(const EditorCamera& camera)
    {
        debug_data.viewProjectionMatrix = camera.getProjection() * camera.getViewMatrix();
        debug_data.lineVertices.clear();
        debug_data.stats = {};
    }

    void DebugRenderer::end()
    {
        flush();
    }

    void DebugRenderer::flush()
    {
        if (debug_data.lineVertices.empty()) { return; }

        drawBatch();
        debug_data.lineVertices.clear();
    }

    void DebugRenderer::drawBatch()
    {
        debug_data.lineShader->bind();
        debug_data.lineShader->uploadMatrix4f(debug_data.viewProjectionMatrix, "u_ViewProj");

        debug_data.lineVertexBuffer->setData(debug_data.lineVertices.data(),debug_data.lineVertices.size() * sizeof(LineVertex));


        debug_data.lineVertexArray->bind();

        glLineWidth(2.0f);
        GraphicsAPI::drawArrays(debug_data.lineVertexArray, static_cast<uint32_t>(debug_data.lineVertices.size()), DrawMode::Lines);


        debug_data.stats.drawCalls++;
    }


    void DebugRenderer::drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& colour)
    {
        if (debug_data.lineVertices.size() >= debug_data.maxVertices - 2)
        {
            flush();
        }

        debug_data.lineVertices.push_back({ start, colour });
        debug_data.lineVertices.push_back({ end, colour });
        debug_data.stats.linesDrawn++;
    }

    void DebugRenderer::drawMeshBoundingBox(const RefPtr<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& colour)
    {
        if (!mesh) { return; }

        const auto& vertices = mesh->getVertices();
        if (vertices.empty()) return;

        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

        // Find min/max in local space
        for (const auto& vertex : vertices)
        {
            min = glm::min(min, vertex.position);
            max = glm::max(max, vertex.position);
        }

        std::vector<glm::vec3> corners = {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, max.z),
            glm::vec3(min.x, max.y, max.z)
        };

        for (auto& corner : corners)
        {
            corner = glm::vec3(transform * glm::vec4(corner, 1.0f));
        }

        drawLine(corners[0], corners[1], colour);
        drawLine(corners[1], corners[2], colour);
        drawLine(corners[2], corners[3], colour);
        drawLine(corners[3], corners[0], colour);

        drawLine(corners[4], corners[5], colour);
        drawLine(corners[5], corners[6], colour);
        drawLine(corners[6], corners[7], colour);
        drawLine(corners[7], corners[4], colour);

        drawLine(corners[0], corners[4], colour);
        drawLine(corners[1], corners[5], colour);
        drawLine(corners[2], corners[6], colour);
        drawLine(corners[3], corners[7], colour);

        debug_data.stats.boxesDrawn++;
    }

    void DebugRenderer::drawBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour)
    {
        drawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), colour);
        drawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), colour);
        drawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), colour);
        drawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z), colour);

        drawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), colour);
        drawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), colour);
        drawLine(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), colour);
        drawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), colour);

        drawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), colour);
        drawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), colour);
        drawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), colour);
        drawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), colour);

        debug_data.stats.boxesDrawn++;
    }

    void DebugRenderer::drawWireCube(const glm::vec3& center, const glm::vec3& size, const glm::vec4& colour)
    {
        glm::vec3 halfSize = size * 0.5f;
        glm::vec3 min = center - halfSize;
        glm::vec3 max = center + halfSize;

        drawBoundingBox(min, max, colour);
    }



    DebugRenderer::Stats DebugRenderer::getStats()
    {
        return debug_data.stats;
    }

    void DebugRenderer::resetStats()
    {
        debug_data.stats = {};
    }
}
