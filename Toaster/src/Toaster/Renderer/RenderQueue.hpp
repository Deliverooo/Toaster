#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include <vector>
#include <unordered_map>
#include <functional>

#include "FrustumCulling.hpp"
#include "Toaster/Scene/Components.hpp"

namespace tst
{
    struct SubMeshRenderCommand
    {
        RefPtr<Mesh> mesh;
        RefPtr<Material> material;
        glm::mat4 transform;
        uint32_t entityId;
        uint32_t submeshIndex;

        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t materialIndex; // Changed from MaterialID to uint32_t to match your existing code

        AABB worldAABB;
        glm::vec3 worldPosition;
        float boundingSphereRadius;
        float distanceToCamera;
        uint32_t sortKey;

        void calcWorldBounds();
        void calcWorldBounds(const AABB& meshAABB);
        uint32_t calcSortKey() const;
    };

    enum class RenderPass : uint16_t
    {
        ZPrePass = 0,
        Opaque = 1,
        Transparent = 2,
        AlphaTest = 3,
        Shadow = 4,
        Lighting = 5,
        PostProcess = 6,
    };

    class RenderQueue
    {
    public:
        struct Stats
        {
            uint32_t totalSubmitted = 0;
            uint32_t culled = 0;
            uint32_t rendered = 0;
            uint32_t frustumTests = 0;
            float avgCullTime = 0.0f;

            void reset()
            {
                totalSubmitted = 0;
                culled = 0;
                rendered = 0;
                frustumTests = 0;
                avgCullTime = 0.0f;
            }
        };

        RenderQueue();

        void setCullingSystem(CullingSystem* system) { m_cullingSystem = system; }

        void submitMesh(const RefPtr<Mesh>& mesh, const glm::mat4& transform, uint32_t entityId, const glm::vec3& cameraPos);
        void submitMeshWithMaterialSlots(const RefPtr<Mesh>& mesh, const glm::mat4& transform, uint32_t entityId, const glm::vec3& cameraPos, const std::vector<MaterialSlot>& materialSlots);

        const Stats& getStats() const { return m_stats; }

        void sort();
        void clear();

        // Get all the render commands for a given render pass
        const std::vector<SubMeshRenderCommand>& getCommands(RenderPass pass) const;

        void renderDebugBoundingBoxes(bool enabled) { m_renderBoundingBoxes = enabled; }
        bool isRenderingBoundingBoxes() const { return m_renderBoundingBoxes; }

    private:
        bool m_renderBoundingBoxes = true;

        CullingSystem* m_cullingSystem = nullptr;

        // maps a render pass to a vector of render commands
        std::unordered_map<RenderPass, std::vector<SubMeshRenderCommand>> m_renderPasses;

        Stats m_stats;

        // Helper method to determine render pass based on material properties
        RenderPass determineRenderPass(const RefPtr<Material>& material) const;

        static bool sortOpaque(const SubMeshRenderCommand& a, const SubMeshRenderCommand& b);
        static bool sortTransparent(const SubMeshRenderCommand& a, const SubMeshRenderCommand& b);
    };
}
